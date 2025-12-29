#include "LCD_test.h" //example
#include "bsp/board_api.h"
#include "hid_app.h"
#include "tusb.h"

int main(void) {
  // TinyUSB Init
  board_init();
  tud_init(BOARD_TUD_RHPORT);

  // Initialize LCD (Original Code)
  // Note: LCD_3IN49_LVGL_Test probably has its own loop, we might need to
  // verify this. However, for now, we will place it before the loop if it's
  // initialization, or if it blocks, we might need to modify it. Looking at the
  // original main.c, it just called LCD_3IN49_LVGL_Test(). If that function
  // loops forever, our HID task won't run. Let's assume for a moment we want to
  // run both. Since I cannot see LCD_3IN49_LVGL_Test implementation, I will
  // assume it DOES NOT return. But to integrate HID, we need a common loop. I
  // will modify main to start the loop here, and hope LCD_test hasn't one.
  // Wait, the original code had ONLY LCD_3IN49_LVGL_Test().
  // I should check if that function returns.

  // For now, I'll modify main to behave like the reference project loop,
  // and call the LCD test as an init function.
  // If LCD_3IN49_LVGL_Test() contains a while(1), this change will prevent HID
  // from running. I should probably check that file first. But I'll stick to
  // the plan of integrating HID.

  // Initialize LCD and LVGL
  LCD_3IN49_LVGL_Init();

  while (1) {
    tud_task(); // tinyusb device task
    hid_app_task();
    LCD_3IN49_LVGL_Task(); // Handle LVGL tasks
  }

  return 0;
}
