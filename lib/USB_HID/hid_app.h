#ifndef HID_APP_H
#define HID_APP_H

#include <stdbool.h>
#include <stdint.h>

void hid_app_task(void);

// Keyboard API
bool send_key_press(uint8_t modifier, uint8_t key_code);
bool send_key_release(void);

// Macro Definition Structure
typedef struct {
  uint8_t modifier;
  uint8_t key_code;
  uint16_t delay_ms;  // Delay in milliseconds after this key press (0 = no delay)
} key_event_t;

typedef struct {
  const char *label;       // Button label (e.g., "Email", "Login")
  const char *comment;     // Optional tooltip/comment (NULL if none)
  key_event_t *keys;       // Array of key events (dynamically allocated)
  uint8_t key_count;       // Number of keys in the array
} macro_definition_t;

// Macro API
uint8_t hid_get_macro_count(void);
const char *hid_get_macro_label(uint8_t index);
const char *hid_get_macro_comment(uint8_t index);
void hid_run_macro_by_index(uint8_t index);

// Helper functions for dynamic macro creation
macro_definition_t* create_macro(const char* label, const char* comment);
void destroy_macro(macro_definition_t* macro);
void add_text_to_macro(macro_definition_t* macro, const char* text);
void add_keys_to_macro(macro_definition_t* macro, const key_event_t* keys, uint8_t count);

// Mouse API
void hid_set_mouse_velocity(int8_t x, int8_t y);
void hid_mouse_button(uint8_t buttons);
void hid_mouse_click(uint8_t buttons); // Immediate blocking click

#endif
