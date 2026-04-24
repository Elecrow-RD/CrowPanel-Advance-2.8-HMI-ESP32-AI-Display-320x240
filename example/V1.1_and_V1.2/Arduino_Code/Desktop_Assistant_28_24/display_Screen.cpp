#include <lvgl.h>
#include "display_Screen.h"
#include "LovyanGFX_Driver.h"
#include <Arduino.h>
#include <Wire.h>

LGFX gfx;

// Screen buffer
#define DRAW_BUF_SIZE (320 * 240 / 10)
lv_color_t draw_buf[DRAW_BUF_SIZE];
static lv_display_t *disp;

// Flush function
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    gfx.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t *)px_map);
    lv_display_flush_ready(disp);
}

// Touch read function
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    if (gfx.getTouch(&data->point.x, &data->point.y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        // Serial.printf("Touch at: (%ld, %ld)\n", data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// Get tick time
uint32_t my_tick() {
    return millis();
}

// Initialize LVGL display
void initLVGL() {
    lv_init();
    lv_tick_set_cb(my_tick);
  
    disp = lv_display_create(320, 240);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
    // Register input device (touch)
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
}

// Initialize display
void initDisplay() {
    Wire.begin(15, 16);
    delay(50);

    gfx.init();
    gfx.initDMA();
    gfx.startWrite();
    gfx.fillScreen(TFT_BLACK);

    initLVGL();

    /* Turn on backlight */
    pinMode(38, OUTPUT);
    digitalWrite(38, HIGH);

    gfx.fillScreen(TFT_BLACK);
}
