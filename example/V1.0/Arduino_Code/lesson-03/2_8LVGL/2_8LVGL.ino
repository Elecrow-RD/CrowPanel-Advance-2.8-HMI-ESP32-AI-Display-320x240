#include "pins_config.h"
#include "LovyanGFX_Driver.h"

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <SPI.h>

#include <stdbool.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include "ui.h"

LGFX gfx;

#include "touch.h"

//UI
#include "ui.h"

/* Change to your screen resolution */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;
static lv_color_t *buf1;

//Display refresh
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  if (gfx.getStartCount() > 0) {
    gfx.endWrite();
  }
  gfx.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::rgb565_t *)&color_p->full);

  lv_disp_flush_ready(disp);  //  Tell lvgl that the refresh is complete
}

uint16_t touchX, touchY;
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_REL;
  if ( gfx.getTouch( &touchX, &touchY ) ) {
    data->state = LV_INDEV_STATE_PR;
    //Set coordinates (the Y-axis of the screen on Longxian is opposite, and the X-axis of the screen on Puyang is opposite)
//    data->point.x = touchX;
//    data->point.y = LCD_V_RES - touchY; //After rotation, the Y-axis is reversed
    data->point.x = LCD_H_RES - touchX;
    data->point.y = touchY;

    Serial.print( "Data x " );
    Serial.println( data->point.x );
    Serial.print( "Data y " );
    Serial.println( data->point.y );
  }
}


void setup()
{
  Serial.begin(115200); 

  pinMode(18, OUTPUT);

  Wire.begin(15, 16);
  delay(50);

 //GT911 power on timing --->0x5D
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  delay(20);
  digitalWrite(2, HIGH);
  delay(100);
  pinMode(1, INPUT);
  /*end*/


  // //  delay(50);
  // Out.reset();
  // Out.setMode(IO_OUTPUT); // Config IO0 of PCA9557 to INPUT mode
  // Out.setState(IO1, IO_HIGH);


  // Init Display
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  lv_init();
  size_t buffer_size = sizeof(lv_color_t) * LCD_H_RES * LCD_V_RES;
  buf = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

  lv_disp_draw_buf_init(&draw_buf, buf, buf1, LCD_H_RES * LCD_V_RES);

  //Initialize display
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  //Change the following lines to your display resolution
  disp_drv.hor_res = LCD_H_RES;
  disp_drv.ver_res = LCD_V_RES;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  //Initialize input device driver program
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  //Scan iic ***
  uint8_t gt911_address;
  delay(100);


  /* Turn on backlight */
  pinMode(38, OUTPUT);  //  Backlight pin
  digitalWrite(38, HIGH);

  // Init touch device
  gt911_address = 0x5D;
  touch_init(gt911_address);

  gfx.fillScreen(TFT_BLACK);
  // lv_demo_widgets();
  ui_init();

  Serial.println( "Setup done" );
}


void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}

