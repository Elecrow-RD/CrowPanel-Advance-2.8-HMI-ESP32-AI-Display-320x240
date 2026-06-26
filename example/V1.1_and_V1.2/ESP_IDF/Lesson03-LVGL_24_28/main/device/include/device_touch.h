#ifndef DEVICE_TOUCH_H_
#define DEVICE_TOUCH_H_

#include "esp_lcd_touch.h"

extern esp_lcd_touch_handle_t tp;
extern esp_lcd_panel_io_handle_t tp_io_handle;

void device_touch_init(void);

#endif
