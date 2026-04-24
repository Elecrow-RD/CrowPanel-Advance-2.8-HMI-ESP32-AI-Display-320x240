/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "tool_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define I2C_MASTER_SDA_IO 15
#define I2C_MASTER_SCL_IO 16
#define I2C_MASTER_FREQ_HZ 400000
i2c_master_bus_handle_t i2c_bus_handle = NULL;
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

/**
 * @brief Initialize the I2C master bus with predefined configurations.
 * @return ESP_OK: I2C master bus initialized successfully. ESP_ERR_INVALID_ARG: I2C bus configuration error. Others: Fail to allocate resources for the I2C bus.
 */
esp_err_t bsp_i2c_init(void)
{
    static esp_err_t err = ESP_OK;
    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,

    };
    err = i2c_new_master_bus(&conf, &i2c_bus_handle);
    if (err != ESP_OK)
        return err;
    return err;
}

/**
 * @brief Register a specific I2C device to the master bus.
 * @param dev_device_address The 7-bit I2C address of the slave device.
 * @return i2c_master_dev_handle_t: Handle to the registered device. NULL: Registration failed (returns 0).
 */
i2c_master_dev_handle_t bsp_i2c_dev_register(uint16_t dev_device_address)
{
    esp_err_t err = ESP_OK;
    i2c_master_dev_handle_t dev_handle = NULL;
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_device_address,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(i2c_bus_handle, &cfg, &dev_handle);
    if (err == ESP_OK)
        return dev_handle;
    return 0;
}

/**
 * @brief Read raw data from an I2C device.
 * @param i2c_dev Handle of the I2C slave device.
 * @param read_buffer Pointer to the buffer to store received data.
 * @param read_size Number of bytes to read.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_receive(i2c_dev, read_buffer, read_size, 1000);
}

/**
 * @brief Write raw data to an I2C device.
 * @param i2c_dev Handle of the I2C slave device.
 * @param write_buffer Pointer to the data to be sent.
 * @param write_size Number of bytes to write.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_write(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size)
{
    return i2c_master_transmit(i2c_dev, write_buffer, write_size, 1000);
}

/**
 * @brief Read data from a specific register of an I2C device.
 * @param i2c_dev Handle of the I2C slave device.
 * @param reg_addr The 8-bit register address to read from.
 * @param read_buffer Pointer to the buffer to store received data.
 * @param read_size Number of bytes to read.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_transmit_receive(i2c_dev, &reg_addr, 1, read_buffer, read_size, 1000);
}

/**
 * @brief Write a single byte of data to a specific register.
 * @param i2c_dev Handle of the I2C slave device.
 * @param reg_addr The 8-bit register address to write to.
 * @param data The data byte to write.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data)
{
    const uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(i2c_dev, write_buf, sizeof(write_buf), 1000);
}

/**
 * @brief Write multiple bytes of data to a specific register.
 * @param i2c_dev Handle of the I2C slave device.
 * @param reg_addr The 8-bit register address to write to.
 * @param write_buffer Pointer to the data buffer to be sent.
 * @param write_size Number of bytes to write.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_write_reg_data(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, const uint8_t *write_buffer, size_t write_size)
{
    esp_err_t err;
    uint8_t *temp_buf;

    if (write_size <= 32) // Use stack for small transfers (< 32 bytes) to avoid heap fragmentation
    {
        uint8_t stack_buf[33];
        stack_buf[0] = reg_addr;
        memcpy(&stack_buf[1], write_buffer, write_size);
        return i2c_master_transmit(i2c_dev, stack_buf, write_size + 1, 1000);
    }

    temp_buf = (uint8_t *)malloc(write_size + 1); // Use heap for larger transfers
    if (temp_buf == NULL)
        return ESP_ERR_NO_MEM;

    temp_buf[0] = reg_addr;
    memcpy(&temp_buf[1], write_buffer, write_size);
    err = i2c_master_transmit(i2c_dev, temp_buf, write_size + 1, 1000);
    free(temp_buf);

    return err;
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/