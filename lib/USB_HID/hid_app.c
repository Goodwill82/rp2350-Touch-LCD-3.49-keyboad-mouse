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
#define MACRO_MAX_STEPS 64
static key_event_t m_macro_queue[MACRO_MAX_STEPS];
static uint8_t m_macro_head = 0;
static uint8_t m_macro_tail = 0;

// Helper macros for defining macro steps more cleanly
#define KEY(k) {0, k}
#define MOD_KEY(m, k) {m, k}

//--------------------------------------------------------------------+
// Macro Definitions (Static Array - will be loaded from SD card later)
//--------------------------------------------------------------------+
static const key_event_t macro_enterr_keys[] = {
    KEY(HID_KEY_E),     KEY(HID_KEY_N),         KEY(HID_KEY_T),
    KEY(HID_KEY_E),     KEY(HID_KEY_R),         KEY(HID_KEY_R),
    KEY(HID_KEY_ENTER), KEY(HID_KEY_BACKSPACE), KEY(HID_KEY_BACKSPACE),
};

static const key_event_t macro_advanced_keys[] = {
    KEY(HID_KEY_ENTER),
    KEY(HID_KEY_TAB),
    KEY(HID_KEY_ENTER),
    KEY(HID_KEY_ENTER),
    MOD_KEY(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_S), // Ctrl+S
};

static const macro_definition_t g_macro_definitions[] = {
    // Simple text macros
    {.label = "Typing", .text = "typing", .keys = NULL, .key_count = 0},
    {.label = "Hi!", .text = "Hi!", .keys = NULL, .key_count = 0},
    {.label = "Email",
     .text = "user@example.com",
     .keys = NULL,
     .key_count = 0},

    // Complex macro with keys only
    {.label = "Enterr",
     .text = NULL,
     .keys = macro_enterr_keys,
     .key_count = 9},

    // Advanced: text + special keys (demonstrates future capability)
    {.label = "Advanced",
     .text = "Hello World",
     .keys = macro_advanced_keys,
     .key_count = 5},
};

#define MACRO_COUNT                                                            \
  (sizeof(g_macro_definitions) / sizeof(g_macro_definitions[0]))

static void macro_queue_add(uint8_t modifier, uint8_t key_code) {
  uint8_t next = (m_macro_head + 1) % MACRO_MAX_STEPS;
  if (next != m_macro_tail) {
    m_macro_queue[m_macro_head].modifier = modifier;
    m_macro_queue[m_macro_head].key_code = key_code;
    m_macro_head = next;
  }
}

// Helper: Convert ASCII character to HID keycode and modifier
static bool ascii_to_hid(char c, uint8_t *modifier, uint8_t *keycode) {
  *modifier = 0;
  *keycode = 0;

  // Lowercase letters
  if (c >= 'a' && c <= 'z') {
    *keycode = HID_KEY_A + (c - 'a');
    return true;
  }
  // Uppercase letters
  if (c >= 'A' && c <= 'Z') {
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_A + (c - 'A');
    return true;
  }
  // Numbers
  if (c >= '1' && c <= '9') {
    *keycode = HID_KEY_1 + (c - '1');
    return true;
  }
  if (c == '0') {
    *keycode = HID_KEY_0;
    return true;
  }

  // Special characters
  switch (c) {
  case ' ':
    *keycode = HID_KEY_SPACE;
    break;
  case '\n':
    *keycode = HID_KEY_ENTER;
    break;
  case '\t':
    *keycode = HID_KEY_TAB;
    break;
  case '-':
    *keycode = HID_KEY_MINUS;
    break;
  case '=':
    *keycode = HID_KEY_EQUAL;
    break;
  case '[':
    *keycode = HID_KEY_BRACKET_LEFT;
    break;
  case ']':
    *keycode = HID_KEY_BRACKET_RIGHT;
    break;
  case '\\':
    *keycode = HID_KEY_BACKSLASH;
    break;
  case ';':
    *keycode = HID_KEY_SEMICOLON;
    break;
  case '\'':
    *keycode = HID_KEY_APOSTROPHE;
    break;
  case '`':
    *keycode = HID_KEY_GRAVE;
    break;
  case ',':
    *keycode = HID_KEY_COMMA;
    break;
  case '.':
    *keycode = HID_KEY_PERIOD;
    break;
  case '/':
    *keycode = HID_KEY_SLASH;
    break;

  // Shifted special characters
  case '!':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_1;
    break;
  case '@':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_2;
    break;
  case '#':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_3;
    break;
  case '$':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_4;
    break;
  case '%':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_5;
    break;
  case '^':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_6;
    break;
  case '&':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_7;
    break;
  case '*':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_8;
    break;
  case '(':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_9;
    break;
  case ')':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_0;
    break;
  case '_':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_MINUS;
    break;
  case '+':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_EQUAL;
    break;
  case '{':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_BRACKET_LEFT;
    break;
  case '}':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_BRACKET_RIGHT;
    break;
  case '|':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_BACKSLASH;
    break;
  case ':':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_SEMICOLON;
    break;
  case '"':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_APOSTROPHE;
    break;
  case '~':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_GRAVE;
    break;
  case '<':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_COMMA;
    break;
  case '>':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_PERIOD;
    break;
  case '?':
    *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    *keycode = HID_KEY_SLASH;
    break;

  default:
    return false;
  }
  return true;
}

// Helper: Type a string (automatically converts ASCII to HID keycodes)
static void macro_type_string(const char *str) {
  uint8_t modifier, keycode;
  while (*str) {
    if (ascii_to_hid(*str, &modifier, &keycode)) {
      macro_queue_add(modifier, keycode);
    }
    str++;
  }
}

// Helper: Add an array of macro steps (for combining strings with special keys)
static void macro_add_steps(const key_event_t *steps, uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    macro_queue_add(steps[i].modifier, steps[i].key_code);
  }
}

//--------------------------------------------------------------------+
// Macro API Implementation
//--------------------------------------------------------------------+

// Get the number of defined macros
uint8_t hid_get_macro_count(void) { return MACRO_COUNT; }

// Get the label for a macro at the given index
const char *hid_get_macro_label(uint8_t index) {
  if (index >= MACRO_COUNT) {
    return NULL;
  }
  return g_macro_definitions[index].label;
}

// Execute a macro by index
void hid_run_macro_by_index(uint8_t index) {
  if (index >= MACRO_COUNT) {
    return;
  }

  const macro_definition_t *macro = &g_macro_definitions[index];

  // First, type any text if present
  if (macro->text != NULL) {
    macro_type_string(macro->text);
  }

  // Then, add any special keys if present
  if (macro->keys != NULL && macro->key_count > 0) {
    macro_add_steps(macro->keys, macro->key_count);
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
