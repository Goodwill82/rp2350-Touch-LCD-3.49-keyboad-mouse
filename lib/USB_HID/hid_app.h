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
} key_event_t;

typedef struct {
  const char *label;       // Button label (e.g., "Email", "Login")
  const char *text;        // Optional: text to type (NULL if not used)
  const key_event_t *keys; // Optional: array of special keys (NULL if not used)
  uint8_t key_count;       // Number of keys in the array
} macro_definition_t;

// Macro API
uint8_t hid_get_macro_count(void);
const char *hid_get_macro_label(uint8_t index);
void hid_run_macro_by_index(uint8_t index);

// Mouse API
void hid_set_mouse_velocity(int8_t x, int8_t y);
void hid_mouse_button(uint8_t buttons);
void hid_mouse_click(uint8_t buttons); // Immediate blocking click

#endif
