#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "setup_gpio.h"
#include "setup_spi.h"
#include "setup_i2c.h"
#include "device_lcd.h"
#include "device_touch.h"
#include "soft_drv_lvgl_port.h"
#include "lv_ui.h"


void app_main(void)
{

    //Setting the pin of the bulb is output mode
    gpio_set_direction(18, GPIO_MODE_OUTPUT);
    //Small screen size necessary initialization pins------------------------------------------------------------
    gpio_set_direction(GPIO_NUM_38, GPIO_MODE_OUTPUT);
    // Set the GPIO pin level to high
    gpio_set_level(GPIO_NUM_38, 1);
//-----------------------------------------------------------------------------------


    setup_gpio_init();
    setup_spi_init();
    setup_i2c_init();
    device_lcd_init();
    device_touch_init();
    soft_drv_lvgl_port_init();
    lv_ui_init();

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}