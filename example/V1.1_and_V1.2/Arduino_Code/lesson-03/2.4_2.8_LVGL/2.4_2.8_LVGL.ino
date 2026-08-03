#include "pins_config.h"
#include "LovyanGFX_Driver.h"

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <SPI.h>

#include <stdbool.h>

#include "ui.h"

LGFX gfx;

// touch.h uses the global gfx object, so it is included after gfx is created.
#include "touch.h"

#include "ui.h"

/*---------------------------------------------------------------
 * LVGL draw buffers
 * The buffers live in PSRAM because a full-screen RGB565 frame for
 * this LCD is larger than the memory that should be reserved for
 * regular stack or small heap allocations.
 *--------------------------------------------------------------*/
static lv_color_t *buf;
static lv_color_t *buf1;

/**
 * @brief Copy rendered LVGL pixels to the LCD.
 *
 * LVGL calls this function whenever a screen area needs to be refreshed.
 * The callback sends the rendered RGB565 pixels through LovyanGFX DMA, then
 * notifies LVGL that the display driver is ready for the next refresh.
 *
 * @param disp LVGL display object that requested the flush.
 * @param area Pixel area that must be updated.
 * @param px_map Rendered pixel buffer supplied by LVGL.
 * @return None.
 */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  if (gfx.getStartCount() > 0) {
    gfx.endWrite();
  }
  gfx.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::rgb565_t *)px_map);

  lv_display_flush_ready(disp);
}

// Stores the latest raw touch coordinate reported by the touch controller.
uint16_t touchX, touchY;

/**
 * @brief Provide touch coordinates to LVGL.
 *
 * LVGL calls this function when it needs the current pointer state. The raw
 * touch coordinates are converted to the display orientation used by this
 * lesson, then reported to LVGL so widgets can respond to taps and drags.
 *
 * @param indev LVGL input device object that requested the read.
 * @param data Output structure filled with touch state and coordinates.
 * @return None.
 */
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_REL;
  if ( gfx.getTouch( &touchX, &touchY ) ) {
    data->state = LV_INDEV_STATE_PR;

    // Match the raw controller coordinates to the screen orientation.
//    data->point.x = touchX;
//    data->point.y = LCD_V_RES - touchY;
    data->point.x = LCD_H_RES - touchX;
    data->point.y = touchY;

    Serial.print( "Data x " );
    Serial.println( data->point.x );
    Serial.print( "Data y " );
    Serial.println( data->point.y );
  }
}

/**
 * @brief Initialize the display, touch controller, LVGL, and lesson UI.
 *
 * Arduino calls this function once after reset. The initialization order is
 * important: power and bus setup come first, then the LCD and LVGL drivers,
 * then the touch input device, and finally the UI objects that appear on the
 * screen.
 *
 * @param None.
 * @return None.
 */
void setup()
{
  Serial.begin(115200); 

  pinMode(18, OUTPUT);

  /*---------------------------------------------------------------
   * Prepare the I2C touch bus and reset sequence
   * The GT911 address is selected by the reset timing. If this order is
   * changed, the touch controller may not respond at the expected address.
   *--------------------------------------------------------------*/
  Wire.begin(15, 16);
  delay(50);

  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  delay(20);
  digitalWrite(2, HIGH);
  delay(100);
  pinMode(1, INPUT);

  /*---------------------------------------------------------------
   * Initialize LCD output and LVGL rendering
   * The display driver must be ready before LVGL starts sending pixel
   * buffers through the flush callback.
   *--------------------------------------------------------------*/
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  lv_init();
  lv_tick_set_cb(millis);
  size_t buffer_size = sizeof(lv_color_t) * LCD_H_RES * LCD_V_RES;
  buf = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

  lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(disp, buf, buf1, buffer_size, LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  /*---------------------------------------------------------------
   * Register touch input with LVGL
   * After this registration, LVGL can ask my_touchpad_read() for the
   * latest press state and route touch events to UI widgets.
   *--------------------------------------------------------------*/
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  uint8_t gt911_address;
  delay(100);

  /*---------------------------------------------------------------
   * Turn on the backlight and create the UI
   * The LCD may be initialized correctly while still appearing black if
   * the backlight pin is not enabled.
   *--------------------------------------------------------------*/
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);

  gt911_address = 0x5D;
  touch_init(gt911_address);

  gfx.fillScreen(TFT_BLACK);
  ui_init();

  Serial.println( "Setup done" );
}


/**
 * @brief Keep LVGL animations, input handling, and redraws running.
 *
 * Arduino calls this function repeatedly after setup() finishes. Calling
 * lv_timer_handler() often keeps the UI responsive while the short delay
 * leaves time for other background tasks.
 *
 * @param None.
 * @return None.
 */
void loop()
{
  lv_timer_handler();
  delay(5);
}
