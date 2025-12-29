#include "hid_app.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "psram_tool.h"
#include "rp_pico_alloc.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>

#include "DEV_Config.h"
#include "LCD_3IN49.h"
#include "Touch.h"
#include "lvgl.h"
#include "qspi_pio.h"

#define DISP_HOR_RES 172
#define DISP_VER_RES 640

// LVGL
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t *buf0;

// Touch
static uint16_t ts_x;
static uint16_t ts_y;
static lv_indev_state_t ts_act;
static lv_indev_drv_t indev_ts;

// Timer
static struct repeating_timer lvgl_timer;

// Forward declarations
static void disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area,
                          lv_color_t *color_p);
static void touch_callback(uint gpio, uint32_t events);
static void ts_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
static void dma_handler(void);
static bool repeating_lvgl_timer_callback(struct repeating_timer *t);

void event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  const char *id = lv_obj_get_user_data(obj);
  if (!id)
    return;

  if (code == LV_EVENT_CLICKED) {
    if (strcmp(id, "M1") == 0)
      hid_run_macro(MACRO_ID_TYPING);
    else if (strcmp(id, "M2") == 0)
      hid_run_macro(MACRO_ID_ENTERR);
    else if (strcmp(id, "M3") == 0)
      hid_run_macro(MACRO_ID_TEST);

    else if (strcmp(id, "LC") == 0) {
      hid_mouse_click(1); // Left click
    } else if (strcmp(id, "RC") == 0) {
      hid_mouse_click(2); // Right click
    }
  } else if (code == LV_EVENT_PRESSED) {
    if (strcmp(id, "UP") == 0)
      hid_set_mouse_velocity(0, -5);
    else if (strcmp(id, "DN") == 0)
      hid_set_mouse_velocity(0, 5);
    else if (strcmp(id, "LT") == 0)
      hid_set_mouse_velocity(-5, 0);
    else if (strcmp(id, "RT") == 0)
      hid_set_mouse_velocity(5, 0);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (strcmp(id, "UP") == 0 || strcmp(id, "DN") == 0 ||
        strcmp(id, "LT") == 0 || strcmp(id, "RT") == 0) {
      hid_set_mouse_velocity(0, 0);
    }
  }
}

void Widgets_Init(void) {
  lv_obj_clean(lv_scr_act());

  lv_obj_t *btn;

  // Title
  lv_obj_t *title = lv_label_create(lv_scr_act());
  lv_label_set_text(title, "HID Control");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Mouse D-Pad - Compact vertical (172px wide screen)
  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 56, 50); // Centered
  lv_obj_set_size(btn, 60, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_user_data(btn, "UP");
  lv_label_set_text(lv_label_create(btn), LV_SYMBOL_UP);

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 6, 105); // Left
  lv_obj_set_size(btn, 60, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_user_data(btn, "LT");
  lv_label_set_text(lv_label_create(btn), LV_SYMBOL_LEFT);

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 106, 105); // Right
  lv_obj_set_size(btn, 60, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_user_data(btn, "RT");
  lv_label_set_text(lv_label_create(btn), LV_SYMBOL_RIGHT);

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 56, 160); // Centered
  lv_obj_set_size(btn, 60, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_user_data(btn, "DN");
  lv_label_set_text(lv_label_create(btn), LV_SYMBOL_DOWN);

  // Mouse Clicks
  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 6, 225);
  lv_obj_set_size(btn, 75, 45);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn, "LC");
  lv_label_set_text(lv_label_create(btn), "L-Clk");

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 91, 225);
  lv_obj_set_size(btn, 75, 45);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn, "RC");
  lv_label_set_text(lv_label_create(btn), "R-Clk");

  // Macro Buttons - Vertical stack
  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 36, 290);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn, "M1");
  lv_label_set_text(lv_label_create(btn), "1: Typing");

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 36, 350);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn, "M2");
  lv_label_set_text(lv_label_create(btn), "2: Enterr");

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_pos(btn, 36, 410);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn, "M3");
  lv_label_set_text(lv_label_create(btn), "3: Test");
}

static void disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area,
                          lv_color_t *color_p) {
  LCD_3IN49_SetWindows(area->x1, area->y1, area->x2 + 1, area->y2 + 1);
  QSPI_Select(qspi);
  QSPI_Pixel_Write(qspi, 0x2c);

  dma_channel_configure(
      dma_tx, &c, &qspi.pio->txf[qspi.sm], color_p,
      ((area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1)) * 2, true);
}

static void touch_callback(uint gpio, uint32_t events) {
  if (gpio == TOUCH_INT_PIN) {
    Touch_Read_State();
    ts_x = TOUCH.Point1_x;
    ts_y = TOUCH.Point1_y;
    ts_act = LV_INDEV_STATE_PRESSED;
  }
}

static void ts_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  data->point.x = ts_x;
  data->point.y = ts_y;
  data->state = ts_act;
  ts_act = LV_INDEV_STATE_RELEASED;
}

static void dma_handler(void) {
  if (dma_channel_get_irq0_status(dma_tx)) {
    dma_channel_acknowledge_irq0(dma_tx);
    QSPI_Deselect(qspi);
    lv_disp_flush_ready(&disp_drv);
  }
}

static bool repeating_lvgl_timer_callback(struct repeating_timer *t) {
  lv_tick_inc(5);
  return true;
}

void LVGL_Init(void) {
  // 1. Init Timer for LVGL tick
  add_repeating_timer_ms(5, repeating_lvgl_timer_callback, NULL, &lvgl_timer);

  // 2. Init LVGL core
  lv_init();

  // 3. Init LVGL display
  buf0 = (lv_color_t *)malloc(DISP_HOR_RES * DISP_VER_RES * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&disp_buf, buf0, NULL, DISP_HOR_RES * DISP_VER_RES);
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = disp_flush_cb;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.hor_res = DISP_HOR_RES;
  disp_drv.ver_res = DISP_VER_RES;
  disp_drv.full_refresh = 1;
  disp_drv.direct_mode = 0;
  lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

  // 4. Init touch screen as input device
  lv_indev_drv_init(&indev_ts);
  indev_ts.type = LV_INDEV_TYPE_POINTER;
  indev_ts.read_cb = ts_read_cb;
  lv_indev_t *ts_indev = lv_indev_drv_register(&indev_ts);

  // Enable touch IRQ
  DEV_KEY_Config(TOUCH_INT_PIN);
  DEV_IRQ_SET(TOUCH_INT_PIN, GPIO_IRQ_EDGE_FALL, &touch_callback);

  // 5. Init DMA for transmit color data from memory to SPI
  dma_channel_set_irq0_enabled(dma_tx, true);
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);
}
