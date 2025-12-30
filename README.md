# RP2350 Touch LCD 3.49" - HID Keyboard & Mouse

USB HID Keyboard and Mouse implementation for the Waveshare RP2350 Touch LCD 3.49" display.
See https://www.waveshare.com/wiki/RP2350-Touch-LCD-3.49 
and https://www.waveshare.com/rp2350-touch-lcd-3.49.htm 
for device. 

## Features

- **USB HID Device**: Enumerates as both keyboard and mouse
- **Touch Screen UI**: LVGL-based interface with:
  - Mouse D-Pad (Up/Down/Left/Right) with continuous movement
  - Left/Right Click buttons
  - Up to 6 Programmable macro buttons with tooltips
- **Advanced Macro System**: 
  - Dynamic macro creation with text, keys, and delays
  - ASCII-to-HID conversion with proper modifier handling
  - Configurable delays between keystrokes
  - Optional comments/tooltips for each macro
  - Future SD card loading support
- **Background Macro Engine**: Queue-based keystroke system prevents missed keys
- **Concurrent Operation**: LCD and USB HID run simultaneously without blocking

## Hardware

- Waveshare RP2350 Touch LCD 3.49" (172x640 AMOLED)
- RP2350 microcontroller
- Capacitive touch screen
- IMU (QMI8658)
- RTC (PCF85063A)

## Macros

The device features a sophisticated macro system with up to 6 programmable macro buttons. Macros support:

- **Text Typing**: Automatic ASCII-to-HID conversion with proper shift handling
- **Special Keys**: ENTER, TAB, ESC, arrow keys, etc.
- **Modifier Combinations**: CTRL+C, ALT+F4, SHIFT+key, etc.
- **Delays**: Configurable pauses between keystrokes (up to 65 seconds)
- **Comments**: Optional tooltips for UI display

### Current Example Macros

1. **"Email Login"**: `"user@example.com" TAB "password" ENTER`
   - *Login to email account with username and password*

2. **"Save Document"**: `CTRL+S`
   - *Save the current document*

3. **"Complex Macro"**: `"Hello World!" DELAY(500) CTRL+A "replaced" ENTER`
   - *Type greeting, wait, select all, replace with 'replaced'*

4. **"Simple Text"**: `"Just type this text"`

### Future: SD Card Macro Loading

Macros can be loaded from human-editable `.txt` files on SD card:

```
Email Login
"user@example.com" TAB "password" ENTER
"Login to email account with username and password"

Save Document
CTRL+S
"Save the current document"
```

**Planned Features:**
- Hot-pluggable macro files
- Multiple macro sets per SD card
- Real-time macro editing and reloading

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
HID implementation inspired by TinyUSB examples and [pico_hid_device](https://github.com/Goodwill82/pico_hid_device).
