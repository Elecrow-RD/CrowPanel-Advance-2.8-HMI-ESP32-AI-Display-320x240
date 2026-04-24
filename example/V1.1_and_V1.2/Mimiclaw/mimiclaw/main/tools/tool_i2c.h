#pragma once

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <stddef.h>
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define I2C_TAG "I2C"
#define I2C_INFO(fmt, ...) ESP_LOGI(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_DEBUG(fmt, ...) ESP_LOGD(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_ERROR(fmt, ...) ESP_LOGE(I2C_TAG, fmt, ##__VA_ARGS__)
extern i2c_master_bus_handle_t i2c_bus_handle;
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

/*——————————————————————————————————————————Function declaration—————————————————————————————————————————*/

/**
 * @brief Initialize the I2C master bus with predefined configurations.
 * @return ESP_OK: I2C master bus initialized successfully. ESP_ERR_INVALID_ARG: I2C bus configuration error. Others: Fail to allocate resources for the I2C bus.
 */
esp_err_t bsp_i2c_init(void);

/**
 * @brief Register a specific I2C device to the master bus.
 * @param dev_device_address The 7-bit I2C address of the slave device.
 * @return i2c_master_dev_handle_t: Handle to the registered device. NULL: Registration failed (returns 0).
 */
i2c_master_dev_handle_t bsp_i2c_dev_register(uint16_t dev_device_address);

/**
 * @brief Read raw data from an I2C device.
 * @param i2c_dev Handle of the I2C slave device.
 * @param read_buffer Pointer to the buffer to store received data.
 * @param read_size Number of bytes to read.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size);

/**
 * @brief Write raw data to an I2C device.
 * @param i2c_dev Handle of the I2C slave device.
 * @param write_buffer Pointer to the data to be sent.
 * @param write_size Number of bytes to write.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_write(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size);

/**
 * @brief Read data from a specific register of an I2C device.
 * @param i2c_dev Handle of the I2C slave device.
 * @param reg_addr The 8-bit register address to read from.
 * @param read_buffer Pointer to the buffer to store received data.
 * @param read_size Number of bytes to read.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size);

/**
 * @brief Write a single byte of data to a specific register.
 * @param i2c_dev Handle of the I2C slave device.
 * @param reg_addr The 8-bit register address to write to.
 * @param data The data byte to write.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data);

/**
 * @brief Write multiple bytes of data to a specific register.
 * @param i2c_dev Handle of the I2C slave device.
 * @param reg_addr The 8-bit register address to write to.
 * @param write_buffer Pointer to the data buffer to be sent.
 * @param write_size Number of bytes to write.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_i2c_write_reg_data(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, const uint8_t *write_buffer, size_t write_size);
/*——————————————————————————————————————————Function declaration end——————————————-——————————————————————*/