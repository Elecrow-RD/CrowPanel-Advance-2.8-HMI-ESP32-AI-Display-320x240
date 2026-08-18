#include "pins_config.h"
#include "LovyanGFX_Driver.h"
#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <SPI.h>
#include <stdbool.h>

#include <Crowbits_DHT20.h>

// Provides LCD drawing, DMA transfer, and display configuration.
LGFX gfx;

// Reads temperature and humidity from the DHT20 sensor on the Wire1 bus.
Crowbits_DHT20 dht20(&Wire1);

#include "touch.h"
#include "ui.h"

/*---------------------------------------------------------------
 * LVGL draw buffers
 * Full-screen buffers are allocated in PSRAM so LVGL can render the
 * interface without exhausting regular internal RAM.
 *--------------------------------------------------------------*/
static lv_color_t *buf;
static lv_color_t *buf1;

/**
 * @brief Copy rendered LVGL pixels to the LCD.
 *
 * LVGL calls this function whenever the UI needs to refresh. The callback
 * forwards the rendered RGB565 pixels to LovyanGFX and then releases LVGL to
 * continue rendering future frames.
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
 * lesson, allowing LVGL widgets to respond to user input.
 *
 * @param indev LVGL input device object that requested the read.
 * @param data Output structure filled with touch state and coordinates.
 * @return None.
 */
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_REL;
  if ( gfx.getTouch( &touchX, &touchY ) ) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = LCD_H_RES - touchX;
    data->point.y = touchY;

    Serial.print( "Data x " );
    Serial.println( data->point.x );
    Serial.print( "Data y " );
    Serial.println( data->point.y );
  }
}
// Optional extension: assign GPIO 18 to an external indicator lamp.
// const int ledPin = 18;

/**
 * @brief Initialize buses, sensors, display, touch input, and the UI.
 *
 * Arduino calls this function once after reset. The setup sequence prepares
 * the shared I2C peripherals, starts the DHT20 sensor, initializes LVGL, and
 * creates the screen objects that will later show temperature and humidity.
 *
 * @param None.
 * @return None.
 */
void setup()
{
  Serial.begin(115200);

  // Optional extension: configure the indicator lamp before using it.
  // pinMode(18, OUTPUT);

  /*---------------------------------------------------------------
   * Start the I2C buses used by this lesson
   * Wire handles onboard peripherals such as touch, while Wire1 is
   * reserved for the DHT20 temperature and humidity sensor.
   *--------------------------------------------------------------*/
  Wire.begin(15, 16);

  Wire1.begin(17, 18);
  delay(50);

  dht20.begin();

  /*---------------------------------------------------------------
   * Select and reset the GT911 touch controller
   * The reset timing places the controller at address 0x5D, which is
   * the address passed to touch_init() later in setup().
   *--------------------------------------------------------------*/
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
   * LVGL needs a display object, draw buffers, and a flush callback
   * before the generated UI can be created.
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
   * After this step, touch points can be routed to the UI controls
   * created by ui_init().
   *--------------------------------------------------------------*/
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  uint8_t gt911_address;
  delay(100);

  /*---------------------------------------------------------------
   * Enable the visible display and create the lesson interface
   * Backlight, touch, and UI initialization must all succeed before
   * the sensor values can be observed on the LCD.
   *--------------------------------------------------------------*/
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);

  gt911_address = 0x5D;
  touch_init(gt911_address);
  gfx.fillScreen(TFT_BLACK);
  ui_init();
  Serial.println( "----- Setup done -----" );
}

/**
 * @brief Read DHT20 values and update the LVGL labels.
 *
 * Arduino calls this function repeatedly after setup() finishes. Each pass
 * reads temperature and humidity, writes the numeric values into the generated
 * UI labels, and lets LVGL process redraw and input events.
 *
 * @param None.
 * @return None.
 */
void loop()
{
  /*---------------------------------------------------------------
   * Sample the DHT20 sensor
   * Integer values keep the generated labels concise for this lesson.
   *--------------------------------------------------------------*/
  char DHT_buffer[10];
  int temp = (int)dht20.getTemperature();
  int humi = (int)dht20.getHumidity();

  snprintf(DHT_buffer, sizeof(DHT_buffer), "%d", temp);
  lv_label_set_text(ui_TempLabel1, DHT_buffer);

  // Optional experiment: drive an external indicator above a temperature limit.
  // if (temp > 30) {
  //   digitalWrite(ledPin, HIGH);
  // } else {
  //   digitalWrite(ledPin, LOW);
  // }
  memset(DHT_buffer, 0, sizeof(DHT_buffer));

  snprintf(DHT_buffer, sizeof(DHT_buffer), "%d", humi);
  lv_label_set_text(ui_HumiLabel2, DHT_buffer);

  // Allow LVGL to process pending redraw and input work on every pass.
  lv_timer_handler();
  delay(5);
}
