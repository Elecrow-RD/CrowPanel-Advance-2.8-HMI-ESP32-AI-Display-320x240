#include "setup_i2c.h"
#include "setup_gpio.h"

#define I2C_MASTER_FREQ_HZ (400000)
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TX_BUF_DISABLE 0

void setup_i2c_init(void)
{
    i2c_config_t i2c_cfg = {};
    i2c_cfg.mode = I2C_MODE_MASTER;
    i2c_cfg.scl_io_num = I2C_TOUCH_SCL;
    i2c_cfg.sda_io_num = I2C_TOUCH_SDA;
    i2c_cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_cfg.master.clk_speed = I2C_MASTER_FREQ_HZ;
    ESP_ERROR_CHECK(i2c_param_config(I2C_TOUCH_PORT, &i2c_cfg));
    i2c_driver_install(I2C_TOUCH_PORT, i2c_cfg.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
