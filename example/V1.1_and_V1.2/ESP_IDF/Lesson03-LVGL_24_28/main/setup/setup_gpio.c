#include "setup_gpio.h"

void setup_gpio_init(void)
{
    gpio_config_t gpio_cfg = {};
    gpio_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_cfg.pin_bit_mask = 1ULL << LCD_BLK_NUM;
    gpio_cfg.pull_down_en = GPIO_PULLUP_DISABLE;
    gpio_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
}