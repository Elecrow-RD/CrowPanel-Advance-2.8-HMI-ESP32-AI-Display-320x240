#pragma once

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_err.h"
#include "esp_log.h"
#include <stddef.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_check.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_ili9488.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "tool_i2c.h"
#include "driver/ledc.h"
#include "soc/clk_tree_defs.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define DISPLAY_TAG "DISPLAY"
#define DISPLAY_INFO(fmt, ...) ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_DEBUG(fmt, ...) ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_ERROR(fmt, ...) ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)

#define USE_BOARD_PANEL_INCH_2_4    1
#define USE_BOARD_PANEL_INCH_2_8    0
#define USE_BOARD_PANEL_INCH_3_5    0


#define SPI_MAX_TRANSFER_SIZE       (32768)
#define BSP_LCD_SPI_NUM             (SPI2_HOST)
#if (USE_BOARD_PANEL_INCH_2_4 || USE_BOARD_PANEL_INCH_2_8)

#define BSP_LCD_IO_SPI_FREQ_HZ      (80 * 1000 * 1000)
#define BSP_H_SIZE                  (320)
#define BSP_V_SIZE                  (240)
#define BSP_LCD_DRAW_BUFF_SIZE      (BSP_H_SIZE * 40)

#elif (USE_BOARD_PANEL_INCH_3_5)

#define BSP_LCD_IO_SPI_FREQ_HZ      (60 * 1000 * 1000)
#define BSP_H_SIZE                  (480)
#define BSP_V_SIZE                  (320)
#define BSP_LCD_DRAW_BUFF_SIZE      (BSP_H_SIZE * 25)

#endif

#define BSP_LCD_TOUCH_POWER_EN  (GPIO_NUM_14)
#define BSP_LCD_IO_SPI_BL       (GPIO_NUM_38)
#define BSP_LCD_PWN_HZ          (30 * 1000)

#define BSP_LCD_IO_SPI_RST      (GPIO_NUM_NC)
#define BSP_LCD_IO_SPI_CS       (GPIO_NUM_40)
#define BSP_LCD_IO_SPI_SCLK     (GPIO_NUM_42)
#define BSP_LCD_IO_SPI_MOSI     (GPIO_NUM_39)
#define BSP_LCD_IO_SPI_MISO     (GPIO_NUM_NC)
#define BSP_LCD_IO_SPI_DC       (GPIO_NUM_41)

#define BSP_TOUCH_ENABLED       0
#define BSP_TOUCH_IO_RST        (GPIO_NUM_48)
#define BSP_TOUCH_IO_INT        (GPIO_NUM_47)

#define BSP_DISPLAY_LVGL_BOUNCE_BUFFER_MODE 0
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

/*——————————————————————————————————————————Function declaration—————————————————————————————————————————*/

#if BSP_TOUCH_ENABLED

/**
 * @brief Reset the touch controller via I2C expansion IO.
 * @return esp_err_t Result of the I2C operation.
 */
esp_err_t bsp_touch_reset();

#endif

/**
 * @brief Sets the LCD backlight brightness using a percentage (0% - 100%).
 * @param percentage Brightness level (0 to 100).
 * @return
 * - ESP_OK: I2C transmission successful.
 * - ESP_ERR_INVALID_ARG: Input percentage out of range.
 * - Others: Underlying I2C communication error.
 */
esp_err_t set_lcd_brightness_percentage(uint8_t percentage);

/**
 * @brief Main entry point to initialize the entire display subsystem.
 * @return
 * - ESP_OK: All display components initialized.
 * - Others: Error code from the failing sub-component.
 */
esp_err_t tool_display_init();

/**
 * @brief Agent Tool: Display text at specific coordinates
 * @param input_json JSON string, e.g., {"x": 100, "y": 200, "content": "Hello"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_display_text_execute(const char *input_json, char *output, size_t output_size);

// /**
//  * @brief Agent Tool: Draw a pixel-like point at specific coordinates
//  * @param input_json JSON string, e.g., {"x": 400, "y": 240}
//  * @param output Result message buffer for the Agent
//  * @param output_size Size of the output buffer
//  * @return ESP_OK on success
//  */
// esp_err_t tool_draw_pixel_execute(const char *input_json, char *output, size_t output_size);

/**
 * @brief Agent Tool: Draw multiple pixel-like points on the canvas
 * @param input_json JSON string, e.g., {"points": [{"x": 100, "y": 100}, {"x": 110, "y": 110}], "color": "blue"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_draw_points_execute(const char *input_json, char *output, size_t output_size);

/**
 * @brief Agent Tool: Draws multiple shapes (lines or polygons) with filling on a Canvas.
 * @param input_json JSON string containing "shapes" array.
 * @param output Buffer for status message.
 * @param output_size Size of the output buffer.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t tool_draw_shapes_execute(const char *input_json, char *output, size_t output_size);

/**
 * @brief Agent Tool: Clears all objects and resets the 800x480 LCD screen
 * @param input_json JSON Data
 * @param output  Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_clear_screen_execute(const char *input_json, char *output, size_t output_size);

/*——————————————————————————————————————————Function declaration end——————————————-——————————————————————*/