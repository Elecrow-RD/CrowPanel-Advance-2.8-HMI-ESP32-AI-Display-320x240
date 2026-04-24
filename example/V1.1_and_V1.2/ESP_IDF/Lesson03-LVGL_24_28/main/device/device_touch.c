#include "device_touch.h"
#include "esp_lcd_touch_xpt2046.h"
#include "setup_gpio.h"
#include "setup_spi.h"
#include "setup_i2c.h"
#include "device_lcd.h"
#include "FT6336U.h"

esp_lcd_touch_handle_t tp = NULL;
esp_lcd_panel_io_handle_t tp_io_handle = NULL;

void device_touch_init(void)
{
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_TOUCH_PORT,
                                             &tp_io_config, &tp_io_handle));
    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max = LCD_V_RES;
    tp_cfg.y_max = LCD_H_RES;
    tp_cfg.rst_gpio_num = I2C_TOUCH_RST;
    tp_cfg.int_gpio_num = I2C_TOUCH_INT;
    tp_cfg.flags.swap_xy = 1;
    tp_cfg.flags.mirror_x = 1;
    tp_cfg.flags.mirror_y = 0;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));
}
