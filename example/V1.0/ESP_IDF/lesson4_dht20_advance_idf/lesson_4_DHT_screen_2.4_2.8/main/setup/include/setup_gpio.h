#ifndef SETUP_GPIO_H_
#define SETUP_GPIO_H_

#include "driver/gpio.h"

// spi lcd pin
#define LCD_BLK_NUM GPIO_NUM_5
#define LCD_SCLK_NUM GPIO_NUM_42
#define LCD_MOSI_NUM GPIO_NUM_39
#define LCD_MISO_NUM GPIO_NUM_NC
#define LCD_RST_NUM -1
#define LCD_DC_NUM GPIO_NUM_41

// spi cs pin
#define LCD_CS_NUM GPIO_NUM_40
// #define LCD_TOUCH_CS_NUM GPIO_NUM_9

// touch i2c pin
#define I2C_TOUCH_SDA GPIO_NUM_15
#define I2C_TOUCH_SCL GPIO_NUM_16

// touch touch pin
#define I2C_TOUCH_INT GPIO_NUM_47
#define I2C_TOUCH_RST GPIO_NUM_48

void setup_gpio_init(void);

#endif
