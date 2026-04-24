/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "tool_led.h"
#include "driver/gpio.h"
#include "cJSON.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define BSP_LED_GPIO 19
static QueueHandle_t _led_queue = NULL;
/**
 * @brief Structure for LED control commands sent via queue
 */
typedef struct
{
    int action;      /*!< 0: OFF, 1: ON, 2: BLINK */
    int interval_ms; /*!< Blink interval in milliseconds */
    int duration_ms; /*!< Total blink duration in milliseconds (-1 for infinite) */
} led_cmd_t;
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

/**
 * @brief LED Execution Task.
 * Handles state transitions and non-blocking blink logic using queue timeouts.
 */
static void led_execute_task(void *pvParameters)
{
    led_cmd_t current_cmd;
    int current_action = 0;
    int elapsed_time = 0;
    bool current_led_state = false;

    while (1)
    {
        TickType_t wait_ticks = portMAX_DELAY;

        if (current_action == 2)
        {
            wait_ticks = current_cmd.interval_ms / portTICK_PERIOD_MS;
            if (wait_ticks == 0)
                wait_ticks = 10;
        }

        led_cmd_t incoming_cmd;
        if (xQueueReceive(_led_queue, &incoming_cmd, wait_ticks) == pdPASS)
        {
            current_cmd = incoming_cmd;
            current_action = current_cmd.action;
            elapsed_time = 0;

            if (current_action == 1)
            {
                current_led_state = true;
                gpio_set_level(BSP_LED_GPIO, 1);
            }
            else if (current_action == 0)
            {
                current_led_state = false;
                gpio_set_level(BSP_LED_GPIO, 0);
            }
            else if (current_action == 2)
            {
                current_led_state = true;
                gpio_set_level(BSP_LED_GPIO, 1);
            }
        }
        else
        {
            if (current_action == 2)
            {
                current_led_state = !current_led_state;
                gpio_set_level(BSP_LED_GPIO, current_led_state);

                if (current_cmd.duration_ms > 0)
                {
                    elapsed_time += current_cmd.interval_ms;
                    if (elapsed_time >= current_cmd.duration_ms)
                    {
                        LED_DEBUG("Blink finished. Auto off");
                        current_action = 0;
                        current_led_state = false;
                        gpio_set_level(BSP_LED_GPIO, 0);
                    }
                }
                else if (current_cmd.duration_ms != -1)
                {
                    LED_ERROR("Invalid duration_ms (%d). Protective stop.", current_cmd.duration_ms);
                    current_action = 0;
                    current_led_state = false;
                    gpio_set_level(BSP_LED_GPIO, 0);
                }
            }
        }
    }
}

/**
 * @brief Initialize LED hardware and the control daemon task
 * @return ESP_OK on success, or ESP error code on failure
 */
esp_err_t tool_led_init()
{
    esp_err_t err = ESP_OK;
    const gpio_config_t gpio_cofig = {
        .pin_bit_mask = 1ULL << BSP_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&gpio_cofig);
    if (err == ESP_OK)
    {
        _led_queue = xQueueCreate(5, sizeof(led_cmd_t));
        xTaskCreatePinnedToCore(led_execute_task, "led execute task", 2048, NULL, 5, NULL, 1);
        LED_DEBUG("LED Queue and Execute Task initialized");
    }
    return err;
}

/**
 * @brief Send a command to the LED control queue
 * @param action 0:OFF, 1:ON, 2:BLINK
 * @param interval_ms Blink speed
 * @param duration_ms Total blink time (-1 for infinite)
 */
static void led_send_cmd(int action, int interval_ms, int duration_ms)
{
    if (_led_queue == NULL)
        return;

    led_cmd_t new_cmd = {
        .action = action,
        .interval_ms = interval_ms,
        .duration_ms = duration_ms};

    xQueueSend(_led_queue, &new_cmd, 0);
}

/**
 * @brief Agent Tool: Execute LED control via JSON input
 * @param input_json JSON string, e.g., {"action": "blink", "interval_ms": 200, "duration_ms": 3000}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_led_control_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *action_obj = cJSON_GetObjectItem(root, "action");
    if (!cJSON_IsString(action_obj))
    {
        snprintf(output, output_size, "Error: 'action' field is required and must be 'on', 'off', or 'blink'");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    const char *action = action_obj->valuestring;
    int interval = 500;
    int duration = 3000;
    int cmd_action = -1;

    if (strcmp(action, "on") == 0)
        cmd_action = 1;
    else if (strcmp(action, "off") == 0)
        cmd_action = 0;
    else if (strcmp(action, "blink") == 0)
    {
        cmd_action = 2;
        cJSON *intv_obj = cJSON_GetObjectItem(root, "interval_ms");
        if (cJSON_IsNumber(intv_obj))
        {
            interval = intv_obj->valueint;
            if (interval < 10)
                interval = 10;
        }

        cJSON *dur_obj = cJSON_GetObjectItem(root, "duration_ms");
        if (cJSON_IsNumber(dur_obj))
        {
            duration = dur_obj->valueint;
            if (duration < -1)
            {
                LED_DEBUG("Agent sent negative duration (%d), defaulting to 3000ms", duration);
                duration = 3000;
            }
        }
    }
    else
    {
        snprintf(output, output_size, "Error: Unknown action '%s'", action);
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    led_send_cmd(cmd_action, interval, duration);
    if (cmd_action == 2)
    {
        if (duration == -1)
            snprintf(output, output_size, "Success: LED is blinking indefinitely at %dms interval", interval);
        else
            snprintf(output, output_size, "Success: LED set to BLINK (%dms interval, %dms duration)", interval, duration);
    }
    else
    {
        snprintf(output, output_size, "Success: LED is now %s", (cmd_action == 1) ? "ON" : "OFF");
    }

    LED_DEBUG("Command executed: %s", output);

    cJSON_Delete(root);
    return ESP_OK;
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/