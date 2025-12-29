#include "hid_app.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// Mouse State
//--------------------------------------------------------------------+
static struct {
  int8_t x;
  int8_t y;
  uint8_t buttons;
  bool dirty;
} m_mouse_state = {0};

void hid_set_mouse_velocity(int8_t x, int8_t y) {
  m_mouse_state.x = x;
  m_mouse_state.y = y;
  m_mouse_state.dirty = true;
}

void hid_mouse_button(uint8_t buttons) {
  m_mouse_state.buttons = buttons;
  m_mouse_state.dirty = true;
}

// Immediate click - sends report right away (blocking)
void hid_mouse_click(uint8_t buttons) {
  // Wait for HID to be ready
  while (!tud_hid_ready()) {
    tud_task();
  }

  // Send button press
  tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, 0, 0, 0, 0);
  sleep_ms(20);

  // Wait again
  while (!tud_hid_ready()) {
    tud_task();
  }

  // Send button release
  tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, 0, 0);
}

//--------------------------------------------------------------------+
// Macro State
//--------------------------------------------------------------------+
typedef struct {
  uint8_t key_code;
  uint8_t modifier;
} key_event_t;

#define MACRO_MAX_STEPS 64
static key_event_t m_macro_queue[MACRO_MAX_STEPS];
static uint8_t m_macro_head = 0;
static uint8_t m_macro_tail = 0;

static void macro_queue_add(uint8_t modifier, uint8_t key_code) {
  uint8_t next = (m_macro_head + 1) % MACRO_MAX_STEPS;
  if (next != m_macro_tail) {
    m_macro_queue[m_macro_head].modifier = modifier;
    m_macro_queue[m_macro_head].key_code = key_code;
    m_macro_head = next;
  }
}

void hid_run_macro(uint8_t macro_id) {
  // Helper macro to add text
  // "typing"
  if (macro_id == MACRO_ID_TYPING) {
    macro_queue_add(0, HID_KEY_T);
    macro_queue_add(0, HID_KEY_Y);
    macro_queue_add(0, HID_KEY_P);
    macro_queue_add(0, HID_KEY_I);
    macro_queue_add(0, HID_KEY_N);
    macro_queue_add(0, HID_KEY_G);
  }
  // "enterr" + Enter + Backspace x2
  else if (macro_id == MACRO_ID_ENTERR) {
    macro_queue_add(0, HID_KEY_E);
    macro_queue_add(0, HID_KEY_N);
    macro_queue_add(0, HID_KEY_T);
    macro_queue_add(0, HID_KEY_E);
    macro_queue_add(0, HID_KEY_R);
    macro_queue_add(0, HID_KEY_R);
    macro_queue_add(0, HID_KEY_ENTER);
    // User wants to back out of the Enter AND the second 'r'.
    macro_queue_add(0, HID_KEY_BACKSPACE);
    macro_queue_add(0, HID_KEY_BACKSPACE);
  } else if (macro_id == MACRO_ID_TEST) {
    macro_queue_add(KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_H); // H
    macro_queue_add(0, HID_KEY_I);                           // i
  }
}

//--------------------------------------------------------------------+
// Helpers
//--------------------------------------------------------------------+
bool send_key_press(uint8_t modifier, uint8_t key_code) {
  uint8_t keycode[6] = {0};
  keycode[0] = key_code;
  return tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
}

bool send_key_release(void) {
  return tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
}

//--------------------------------------------------------------------+
// Task
//--------------------------------------------------------------------+
void hid_app_task(void) {
  // Poll every 10ms for Mouse/Macro
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms)
    return;
  start_ms += interval_ms;

  if (!tud_hid_ready())
    return;

  // 1. Handle Macros (Keyboard)
  // State 0: Idle, State 1: Pressing, State 2: Waiting
  static uint8_t kbd_state = 0;
  static key_event_t current_key;

  if (kbd_state == 1) { // Key is currently pressed
    send_key_release(); // Release it
    kbd_state = 0;      // Back to idle (implied wait?)
  } else if (kbd_state == 0) {
    if (m_macro_head != m_macro_tail) {
      // Get next key
      current_key = m_macro_queue[m_macro_tail];
      m_macro_tail = (m_macro_tail + 1) % MACRO_MAX_STEPS;

      send_key_press(current_key.modifier, current_key.key_code);
      kbd_state = 1; // Mark as pressed
    }
  }

  // 2. Handle Mouse
  // If dirty (button change) OR velocity is non-zero, report
  if (m_mouse_state.dirty || m_mouse_state.x != 0 || m_mouse_state.y != 0) {
    tud_hid_mouse_report(REPORT_ID_MOUSE, m_mouse_state.buttons,
                         m_mouse_state.x, m_mouse_state.y, 0, 0);
    m_mouse_state.dirty =
        false; // Clear dirty flag, but keep reporting if x/y are moving
  }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}
