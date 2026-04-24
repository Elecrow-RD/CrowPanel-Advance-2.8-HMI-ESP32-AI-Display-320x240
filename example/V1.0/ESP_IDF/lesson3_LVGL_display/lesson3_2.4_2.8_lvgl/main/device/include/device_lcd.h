#ifndef DEVICE_LCD_H_
#define DEVICE_LCD_H_

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#define LCD_DRAW_BUFF_HEIGHT (LCD_BUFFER_H_DATA)
#define LCD_BL_NO_LEVEL (1)
#define LCD_H_RES (LCD_H_RES_DATA)
#define LCD_V_RES (240)
#define LCD_DEAW_BUFF_DOUBLE (1)

extern esp_lcd_panel_io_handle_t lcd_io;
extern esp_lcd_panel_handle_t lcd_panel;

void device_lcd_init(void);

#endif
