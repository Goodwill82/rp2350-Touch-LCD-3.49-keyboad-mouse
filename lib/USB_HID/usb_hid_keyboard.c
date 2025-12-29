#include "usb_hid_keyboard.h"
#include "tusb.h"
#include <stdio.h>

/**
 * USB HID Keyboard implementation using TinyUSB
 * Sends keyboard reports to the host via USB HID interface
 */

void usb_hid_keyboard_init(void)
{
    // TinyUSB is initialized by pico_stdio_usb
    // Wait for device to be mounted on host
    printf("[USB HID] Keyboard driver initialized\n");
}

void usb_hid_keyboard_send_key(uint8_t keycode)
{
    // Press and release a key with a small delay between
    usb_hid_keyboard_press_key(keycode);
    // Delay (~100k cycles)
    for (volatile int i = 0; i < 100000; i++);
    usb_hid_keyboard_release_key();
}

void usb_hid_keyboard_press_key(uint8_t keycode)
{
    // Wait until device is ready and mounted
    if (!tud_hid_ready()) {
        printf("[USB HID] Device not ready\n");
        return;
    }

    printf("[USB HID] Pressing key 0x%02x\n", keycode);
    
    // Build 6-byte keyboard report: [reserved, keycode1-5]
    uint8_t keycode_arr[6] = {0};
    keycode_arr[0] = keycode;  // First keycode slot

    // Send HID keyboard report
    // report_id = 0 (standard HID keyboard)
    // modifier = 0 (no shift, ctrl, alt, etc.)
    // keycode_arr contains the key codes
    tud_hid_keyboard_report(0, 0, keycode_arr);
}

void usb_hid_keyboard_release_key(void)
{
    // Wait until device is ready
    if (!tud_hid_ready()) {
        return;
    }

    printf("[USB HID] Releasing all keys\n");
    
    // Send all zeros to release all keys
    uint8_t keycode_arr[6] = {0};
    tud_hid_keyboard_report(0, 0, keycode_arr);
}
