#include "soft_drv_lvgl_port.h"
#include "setup_gpio.h"
#include "setup_spi.h"
#include "device_lcd.h"
#include "device_touch.h"

static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

void soft_drv_lvgl_port_init(void)
{
    /* LCD backlight on */
    ESP_ERROR_CHECK(gpio_set_level(LCD_BLK_NUM, LCD_BL_NO_LEVEL));

    // set lvgl task
    lvgl_port_cfg_t lvgl_cfg = {};
    lvgl_cfg.task_priority = 4;       /* LVGL task priority */
    lvgl_cfg.task_stack = 4 * 1024;   /* LVGL task stack size */
    lvgl_cfg.task_affinity = -1;      /* LVGL task pinned to core (-1 is no affinity) */
    lvgl_cfg.task_max_sleep_ms = 500; /* Maximum sleep in LVGL task */
    lvgl_cfg.timer_period_ms = 5;     /* LVGL timer tick period in ms */
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // set display config
    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle = lcd_io;
    disp_cfg.panel_handle = lcd_panel;
    disp_cfg.buffer_size = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT;
    disp_cfg.double_buffer = LCD_DEAW_BUFF_DOUBLE;
    disp_cfg.hres = LCD_H_RES;
    disp_cfg.vres = LCD_V_RES;
    disp_cfg.monochrome = false;
#if LVGL_VERSION_MAJOR >= 9
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
#endif
    disp_cfg.rotation.swap_xy = true;
    disp_cfg.rotation.mirror_x = true;
    disp_cfg.rotation.mirror_y = false;
    disp_cfg.flags.buff_dma = true;
#if LVGL_VERSION_MAJOR >= 9
    disp_cfg.flags.swap_bytes = true;
#endif
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    // set lcd touch config
    lvgl_port_touch_cfg_t touch_cfg = {};
    touch_cfg.disp = lvgl_disp;
    touch_cfg.handle = tp;
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
}