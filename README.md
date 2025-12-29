# RP2350 Touch LCD 3.49" - HID Keyboard & Mouse

USB HID Keyboard and Mouse implementation for the Waveshare RP2350 Touch LCD 3.49" display.

## Features

- **USB HID Device**: Enumerates as both keyboard and mouse
- **Touch Screen UI**: LVGL-based interface with:
  - Mouse D-Pad (Up/Down/Left/Right) with continuous movement
  - Left/Right Click buttons
  - 3 Programmable macro buttons
- **Background Macro Engine**: Queue-based keystroke system prevents missed keys
- **Concurrent Operation**: LCD and USB HID run simultaneously without blocking

## Hardware

- Waveshare RP2350 Touch LCD 3.49" (172x640 AMOLED)
- RP2350 microcontroller
- Capacitive touch screen
- IMU (QMI8658)
- RTC (PCF85063A)

## Macros

The three macro buttons are pre-programmed:

1. **"Typing"**: Types the word "typing"
2. **"Enterr"**: Types "enterr", presses Enter, then backspaces twice to correct
3. **"Test"**: Types "Hi"

## Building

Requires:
- Pico SDK 2.2.0+
- CMake 3.13+
- ARM GCC toolchain

```bash
mkdir build
cd build
cmake ..
make
```

Flash the generated `.uf2` file to your RP2350 board.

## Project Structure

- `lib/USB_HID/` - HID implementation (keyboard/mouse/macros)
- `examples/src/LVGL_example.c` - Touch UI implementation
- `lib/LCD/` - Display drivers
- `lib/Touch/` - Touch screen drivers
- `lib/lvgl/` - LVGL graphics library

## Credits

Based on Waveshare's RP2350-Touch-LCD-3.49-LVGL demo.
HID implementation inspired by TinyUSB examples.
