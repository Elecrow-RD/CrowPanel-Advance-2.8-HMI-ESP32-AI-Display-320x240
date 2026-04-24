#include "device_lcd.h"
#include "setup_gpio.h"
#include "setup_spi.h"
#include "esp_lcd_ili9341.h"

#define LCD_PIXEL_CLK_HZ (40 * 1000 * 1000)
#define LCD_CMD_BITS (8)
#define LCD_PARAM_BITS (8)

#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_RGB
#define LCD_BITS_PER_PIXEL (16)

esp_lcd_panel_io_handle_t lcd_io = NULL;
esp_lcd_panel_handle_t lcd_panel = NULL;

void device_lcd_init(void)
{
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.dc_gpio_num = LCD_DC_NUM;
    io_config.cs_gpio_num = LCD_CS_NUM;
    io_config.pclk_hz = LCD_PIXEL_CLK_HZ;
    io_config.lcd_cmd_bits = LCD_CMD_BITS;
    io_config.lcd_param_bits = LCD_PARAM_BITS;
    io_config.spi_mode = 0;
    io_config.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI2_HOST, &io_config, &lcd_io));

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_RST_NUM;
    panel_config.rgb_ele_order = LCD_COLOR_SPACE;
    panel_config.bits_per_pixel = LCD_BITS_PER_PIXEL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel));
    // ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(lcd_io, &panel_config, &lcd_panel));

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_set_gap(lcd_panel, 0, 0); 
    esp_lcd_panel_invert_color(lcd_panel, true);
    // esp_lcd_panel_mirror(lcd_panel, false, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);
}
