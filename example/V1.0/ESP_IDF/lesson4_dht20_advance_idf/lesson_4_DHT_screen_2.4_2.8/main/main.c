#include "main.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "setup_gpio.h"
#include "setup_spi.h"
#include "setup_i2c.h"
#include "device_lcd.h"
#include "device_touch.h"
#include "soft_drv_lvgl_port.h"
#include "lv_ui.h"

// Define font (adjust according to the linked font)
#ifndef LV_FONT_MONTSERRAT_14
#define LV_FONT_MONTSERRAT_24 1
#endif

/* Global variables ----------------------------------------------------------------*/
TaskHandle_t read_data_h;
static SemaphoreHandle_t data_mutex = NULL;

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

static sensor_data_t measurements = {0};

/* LVGL objects ----------------------------------------------------------------*/
static lv_obj_t *label_temp;
static lv_obj_t *label_humid;

/* Function declarations ----------------------------------------------------------------*/
void init_screen(void);
void update_display_task(void *arg);

/* Screen initialization --------------------------------------------------------------*/
void init_screen(void)
{
    // Hardware Initialization
    gpio_set_direction(GPIO_NUM_38, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_38, 1);
    
    setup_gpio_init();
    setup_spi_init();
    device_lcd_init();
    soft_drv_lvgl_port_init();
    
    // Create UI elements
    lv_ui_init();  // Keep the original UI initialization
    
    // Temperature label
    label_temp = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0x000000), LV_STATE_DEFAULT);
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 130, 60);
    
    // Humidity label
    label_humid = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_humid, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_humid, lv_color_hex(0x000000), LV_STATE_DEFAULT);
    lv_obj_align(label_humid, LV_ALIGN_TOP_LEFT, 130, 135);
}

/* Display update function ------------------------------------------------------------*/
void update_display_task(void *arg)
{
    (void)arg;
    char buffer[16];
    
    while(1) {
        if(xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            //  Update temperature
            snprintf(buffer, sizeof(buffer), "%.1f", measurements.temperature);
            lv_label_set_text(label_temp, buffer);
            
            // Update humidity
            snprintf(buffer, sizeof(buffer), "%.1f", measurements.humidity);
            lv_label_set_text(label_humid, buffer);
            
            xSemaphoreGive(data_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 500 ms refresh interval
    }
}

/* Sensor reading task ----------------------------------------------------------*/
void dht20_read_task(void *arg)
{
    (void)arg;
    dht20_data_t raw_data;
    
    while(1) {
        if(dht20_read_data(&raw_data) == ESP_OK) {
            if(xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                measurements.temperature = raw_data.temperature;
                measurements.humidity = raw_data.humidity;
                xSemaphoreGive(data_mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2-second reading interval
    }
}

/* Main program -----------------------------------------------------------------*/
void app_main(void)
{
    // Hardware Initialization
    init_screen();
    dht20_begin();
    
    // Create mutex
    data_mutex = xSemaphoreCreateMutex();
    
    // Create tasks
    xTaskCreate(dht20_read_task, "sensor", 2048, NULL, 3, &read_data_h);
    xTaskCreate(update_display_task, "display", 2048, NULL, 2, NULL);
    
    // Keep main loop idle
    while(1) {
        vTaskDelay(portMAX_DELAY);
    }
}