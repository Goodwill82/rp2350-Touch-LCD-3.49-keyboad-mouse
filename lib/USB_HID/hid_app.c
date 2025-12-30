#include "hid_app.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include <stdlib.h>

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
#define MACRO_MAX_STEPS 128
static key_event_t m_macro_queue[MACRO_MAX_STEPS];
static uint8_t m_macro_head = 0;
static uint8_t m_macro_tail = 0;

// Helper macros for defining macro steps more cleanly
#define KEY(k) {0, k, 0}
#define MOD_KEY(m, k) {m, k, 0}
#define DELAY(ms) {0, 0, ms}

//--------------------------------------------------------------------+
// Macro Definitions (Dynamic - will be loaded from SD card later)
//--------------------------------------------------------------------+
#define MACRO_COUNT 4
static macro_definition_t* g_macro_definitions[MACRO_COUNT];
static bool g_macros_initialized = false;

static void initialize_macros(void) {
  if (g_macros_initialized) return;

  // Create dynamic macros based on the comment examples
  g_macro_definitions[0] = create_macro("Email Login", "Login to email account with username and password");
  add_text_to_macro(g_macro_definitions[0], "user@example.com");
  add_keys_to_macro(g_macro_definitions[0], (key_event_t[]){KEY(HID_KEY_TAB)}, 1);
  add_text_to_macro(g_macro_definitions[0], "password");
  add_keys_to_macro(g_macro_definitions[0], (key_event_t[]){KEY(HID_KEY_ENTER)}, 1);

  g_macro_definitions[1] = create_macro("Save Document", "Save the current document");
  add_keys_to_macro(g_macro_definitions[1], (key_event_t[]){MOD_KEY(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_S)}, 1);

  g_macro_definitions[2] = create_macro("Complex Macro", "Type greeting, wait, select all, replace with 'replaced'");
  add_text_to_macro(g_macro_definitions[2], "Hello World!");
  add_keys_to_macro(g_macro_definitions[2], (key_event_t[]){DELAY(500)}, 1);
  add_keys_to_macro(g_macro_definitions[2], (key_event_t[]){MOD_KEY(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_A)}, 1);
  add_text_to_macro(g_macro_definitions[2], "replaced");
  add_keys_to_macro(g_macro_definitions[2], (key_event_t[]){KEY(HID_KEY_ENTER)}, 1);

  g_macro_definitions[3] = create_macro("Simple Text", NULL);
  add_text_to_macro(g_macro_definitions[3], "Just type this text");

  g_macros_initialized = true;
}

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

// Helper functions for dynamic macro creation
macro_definition_t* create_macro(const char* label, const char* comment) {
  if (!label) return NULL;

  macro_definition_t* macro = malloc(sizeof(macro_definition_t));
  if (!macro) return NULL;

  macro->label = label;
  macro->comment = comment; // Can be NULL
  macro->key_count = 0;
  macro->keys = malloc(sizeof(key_event_t) * MACRO_MAX_STEPS);
  if (!macro->keys) {
    free(macro);
    return NULL;
  }

  return macro;
}

void add_text_to_macro(macro_definition_t* macro, const char* text) {
  if (!macro || !text) return;

  uint8_t modifier, keycode;
  while (*text && macro->key_count < MACRO_MAX_STEPS) {
    if (ascii_to_hid(*text, &modifier, &keycode)) {
      macro->keys[macro->key_count].modifier = modifier;
      macro->keys[macro->key_count].key_code = keycode;
      macro->keys[macro->key_count].delay_ms = 0;
      macro->key_count++;
    }
    text++;
  }
}

void add_keys_to_macro(macro_definition_t* macro, const key_event_t* keys, uint8_t count) {
  if (!macro || !keys) return;

  for (uint8_t i = 0; i < count && macro->key_count < MACRO_MAX_STEPS; i++) {
    macro->keys[macro->key_count++] = keys[i];
  }
}

void destroy_macro(macro_definition_t* macro) {
  if (macro) {
    if (macro->keys) free(macro->keys);
    free(macro);
  }
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
uint8_t hid_get_macro_count(void) { 
  initialize_macros();
  return MACRO_COUNT; 
}

// Get the label for a macro at the given index
const char *hid_get_macro_label(uint8_t index) {
  initialize_macros();
  if (index >= MACRO_COUNT) {
    return NULL;
  }
  return g_macro_definitions[index]->label;
}

// Get the comment for a macro at the given index
const char *hid_get_macro_comment(uint8_t index) {
  initialize_macros();
  if (index >= MACRO_COUNT) {
    return NULL;
  }
  return g_macro_definitions[index]->comment;
}

// Execute a macro by index
void hid_run_macro_by_index(uint8_t index) {
  initialize_macros();
  if (index >= MACRO_COUNT) {
    return;
  }

  const macro_definition_t *macro = g_macro_definitions[index];

  // Add all keys to the queue
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
  initialize_macros(); // Ensure macros are initialized

  // Poll every 10ms for Mouse/Macro
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms)
    return;
  start_ms += interval_ms;

  if (!tud_hid_ready())
    return;

  // 1. Handle Macros (Keyboard)
  // State 0: Idle, State 1: Pressing, State 2: Waiting for delay
  static uint8_t kbd_state = 0;
  static key_event_t current_key;
  static uint32_t delay_start_ms = 0;

  if (kbd_state == 1) { // Key is currently pressed
    send_key_release(); // Release it
    if (current_key.delay_ms > 0) {
      kbd_state = 2; // Delay state
      delay_start_ms = board_millis();
    } else {
      kbd_state = 0; // Back to idle
    }
  } else if (kbd_state == 2) { // Delaying
    if (board_millis() - delay_start_ms >= current_key.delay_ms) {
      kbd_state = 0; // Delay done, back to idle
    }
  } else if (kbd_state == 0) {
    if (m_macro_head != m_macro_tail) {
      // Get next key
      current_key = m_macro_queue[m_macro_tail];
      m_macro_tail = (m_macro_tail + 1) % MACRO_MAX_STEPS;

      if (current_key.key_code == 0 && current_key.modifier == 0 && current_key.delay_ms > 0) {
        // This is a delay event - no key press, just delay
        kbd_state = 2;
        delay_start_ms = board_millis();
      } else {
        // Normal key press
        send_key_press(current_key.modifier, current_key.key_code);
        kbd_state = 1; // Mark as pressed
      }
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
