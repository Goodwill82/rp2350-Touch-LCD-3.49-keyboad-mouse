#ifndef HID_APP_H
#define HID_APP_H

#include <stdbool.h>
#include <stdint.h>

void hid_app_task(void);

// Keyboard API
bool send_key_press(uint8_t modifier, uint8_t key_code);
bool send_key_release(void);

// Macro API
#define MACRO_ID_TYPING 1
#define MACRO_ID_ENTERR 2
#define MACRO_ID_TEST 3
void hid_run_macro(uint8_t macro_id);

// Mouse API
void hid_set_mouse_velocity(int8_t x, int8_t y);
void hid_mouse_button(uint8_t buttons);
void hid_mouse_click(uint8_t buttons); // Immediate blocking click

#endif
