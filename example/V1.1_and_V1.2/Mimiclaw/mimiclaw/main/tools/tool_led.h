#pragma once
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define LED_TAG "LED"
#define LED_INFO(fmt, ...) ESP_LOGI(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_DEBUG(fmt, ...) ESP_LOGD(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_ERROR(fmt, ...) ESP_LOGE(LED_TAG, fmt, ##__VA_ARGS__)
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

/*——————————————————————————————————————————Function declaration—————————————————————————————————————————*/
/**
 * @brief Initialize LED hardware and the control daemon task
 * @return ESP_OK on success, or ESP error code on failure
 */
esp_err_t tool_led_init();

/**
 * @brief Agent Tool: Execute LED control via JSON input
 * @param input_json JSON string, e.g., {"action": "blink", "interval_ms": 200, "duration_ms": 3000}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_led_control_execute(const char *input_json, char *output, size_t output_size);
/*——————————————————————————————————————————Function declaration end——————————————-——————————————————————*/