#ifndef USB_HID_KEYBOARD_H
#define USB_HID_KEYBOARD_H

#include <stdint.h>

/**
 * Initialize USB HID keyboard device
 * Call this once during startup
 */
void usb_hid_keyboard_init(void);

/**
 * Send a keyboard key press and release
 * @param keycode USB HID keycode (e.g., HID_KEY_A, HID_KEY_B, etc.)
 */
void usb_hid_keyboard_send_key(uint8_t keycode);

/**
 * Send a keyboard key press (held down)
 * @param keycode USB HID keycode
 */
void usb_hid_keyboard_press_key(uint8_t keycode);

/**
 * Release a held keyboard key (send all zeros)
 */
void usb_hid_keyboard_release_key(void);

#endif // USB_HID_KEYBOARD_H
