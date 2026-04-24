/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "tool_display.h"
#include "cJSON.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static lv_display_t *my_lvgl_disp = NULL;
#if BSP_TOUCH_ENABLED
static esp_lcd_touch_handle_t touch_handle = NULL;
static esp_lcd_panel_io_handle_t tp_io_handle = NULL;
static lv_indev_t *my_touch_indev = NULL;
#endif
static lv_obj_t *global_canvas = NULL;
static lv_color_t *canvas_buf = NULL;
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

/**
 * @brief Helper: Convert color string to lv_color_t.
 * @param color_str Color name (e.g., "red", "pink").
 * @return lv_color_t Corresponding LVGL color object.
 */
static lv_color_t get_lv_color_from_str(const char *color_str)
{
    if (!color_str)
        return lv_color_white();
    if (strcmp(color_str, "red") == 0)
        return lv_palette_main(LV_PALETTE_RED);
    if (strcmp(color_str, "green") == 0)
        return lv_palette_main(LV_PALETTE_GREEN);
    if (strcmp(color_str, "blue") == 0)
        return lv_palette_main(LV_PALETTE_BLUE);
    if (strcmp(color_str, "yellow") == 0)
        return lv_palette_main(LV_PALETTE_YELLOW);
    if (strcmp(color_str, "pink") == 0)
        return lv_color_make(0xFF, 0xC0, 0xCB);
    if (strcmp(color_str, "black") == 0)
        return lv_color_black();
    return lv_color_white();
}

static inline int16_t clamp_i16(int16_t val, int16_t min, int16_t max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

esp_err_t lcd_backlight_init(void)
{
    esp_err_t err = ESP_OK;
    
    // const gpio_config_t gpio_cofig = {
    //     .pin_bit_mask = (1ULL << BSP_LCD_IO_SPI_BL),
    //     .mode = GPIO_MODE_OUTPUT,
    //     .pull_up_en = false,
    //     .pull_down_en = false,
    //     .intr_type = GPIO_INTR_DISABLE,
    // };
    // err = gpio_config(&gpio_cofig);
    // if (err != ESP_OK)
    //     return err;
    const ledc_timer_config_t timer_config = {
        .clk_cfg = LEDC_USE_APB_CLK,
        .duty_resolution = LEDC_TIMER_11_BIT,
        .freq_hz = BSP_LCD_PWN_HZ,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };
    const ledc_channel_config_t channel_config = {
        .gpio_num = BSP_LCD_IO_SPI_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_timer_config(&timer_config);
    if (err != ESP_OK)
        return err;
    err = ledc_channel_config(&channel_config);
    if (err != ESP_OK)
        return err;
    return err;
}

/**
 * @brief Sets the LCD backlight brightness using a percentage (0% - 100%).
 * @param percentage Brightness level (0 to 100).
 * @return
 * - ESP_OK: I2C transmission successful.
 * - ESP_ERR_INVALID_ARG: Input percentage out of range.
 * - Others: Underlying I2C communication error.
 */
esp_err_t set_lcd_brightness_percentage(uint8_t percentage)
{
    if (percentage > 100)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = ESP_OK;
    if (percentage != 0)
    {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, ((percentage * 18) + 200));
        if (err != ESP_OK)
            return err;
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        if (err != ESP_OK)
            return err;
    }
    else
    {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        if (err != ESP_OK)
            return err;
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        if (err != ESP_OK)
            return err;
    }
    return err;
}

#if BSP_TOUCH_ENABLED

/**
 * @brief Initialize the touch controller (GT911) hardware and driver.
 * @note This function handles I2C address probing (Backup and Primary addresses).
 * @return
 * - ESP_OK: Touch driver initialized successfully.
 * - Others: Failed to create I2C panel IO or touch instance.
 */
static esp_err_t touch_init(void)
{
    esp_err_t err = ESP_OK;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .flags =
            {
                .disable_control_phase = 1,
            },
        .scl_speed_hz = 400000,
    };
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_H_SIZE,
        .y_max = BSP_V_SIZE,
        .rst_gpio_num = BSP_TOUCH_IO_RST,
        .int_gpio_num = BSP_TOUCH_IO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
    };
    tp_cfg.driver_data = (void*)&io_config.dev_addr;
    err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle, &io_config, &tp_io_handle);
    if (err != ESP_OK)
        return err;
    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
    return err;
}

#endif

/**
 * @brief Configure and install the RGB LCD panel driver.
 * @note Framebuffer is allocated in PSRAM for ESP32S3 architecture.
 * @return
 * - ESP_OK: RGB panel initialized successfully.
 * - Others: Allocation or configuration error.
 */
static esp_err_t display_port_init(void)
{
    esp_err_t ret = ESP_OK;

    lcd_backlight_init();
    if (ret != ESP_OK) {
        DISPLAY_ERROR("lcd backlight initialization failed");
        return ret;
    }
    
    /* LCD initialization */
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_IO_SPI_SCLK,
        .mosi_io_num = BSP_LCD_IO_SPI_MOSI,
        .miso_io_num = BSP_LCD_IO_SPI_MISO,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), DISPLAY_TAG, "SPI init failed");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_IO_SPI_DC,
        .cs_gpio_num = BSP_LCD_IO_SPI_CS,
        .pclk_hz = BSP_LCD_IO_SPI_FREQ_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &lcd_io_handle), err,
                      DISPLAY_TAG, "New panel IO failed");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_IO_SPI_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
#if (USE_BOARD_PANEL_INCH_2_4 || USE_BOARD_PANEL_INCH_2_8)
        .bits_per_pixel = 16,
#elif (USE_BOARD_PANEL_INCH_3_5)
        .bits_per_pixel = 18,
#endif
    };

#if (USE_BOARD_PANEL_INCH_2_4 || USE_BOARD_PANEL_INCH_2_8)
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io_handle, &panel_config, &panel_handle), err, DISPLAY_TAG, "New panel failed");
#elif (USE_BOARD_PANEL_INCH_3_5)
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ili9488(lcd_io_handle, &panel_config, BSP_LCD_DRAW_BUFF_SIZE, &panel_handle), err, DISPLAY_TAG, "New panel failed");
    // ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ili9486(lcd_io_handle, &panel_config, &panel_handle), err, DISPLAY_TAG, "New panel failed");
#endif

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);
    esp_lcd_panel_invert_color(panel_handle, true);
    // esp_lcd_panel_swap_xy(panel_handle, true);
    // esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_set_gap(panel_handle, 0, 0);

    ESP_LOGI(DISPLAY_TAG, "LCD driver installation success!");
    return ret;

err:
    ESP_LOGI(DISPLAY_TAG, "LCD driver installation failed!");
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
    }
    if (lcd_io_handle) {
        esp_lcd_panel_io_del(lcd_io_handle);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

/**
 * @brief Initialize the LVGL graphics library and register display/input drivers.
 * @note Task is pinned to Core 1 to avoid interference with Core 0 networking.
 * @return
 * - ESP_OK: LVGL system ready.
 * - ESP_FAIL: Failed to add display or touch interface to LVGL.
 */
static esp_err_t lvgl_init()
{
    esp_err_t err = ESP_OK;
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 3,       /* LVGL task priority */
        .task_stack = 8192,       /* LVGL task stack size */
        .task_affinity = 1,       /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 20,    /* LVGL timer tick period in ms */
    };
    err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK)
    {
        DISPLAY_ERROR("LVGL port initialization failed");
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io_handle,
        .panel_handle = panel_handle,
        .control_handle = NULL,
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = false,
        .hres = BSP_H_SIZE,
        .vres = BSP_V_SIZE,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = true,
            .mirror_x = false,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
            .full_refresh = false,
            .direct_mode = false,
        },
    };

    my_lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    if (my_lvgl_disp == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL display port add fail");
    }
#if BSP_TOUCH_ENABLED
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = my_lvgl_disp,
        .handle = touch_handle,
    };
    my_touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (my_touch_indev == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL touch port add fail");
    }
#endif
    if (lvgl_port_lock(0))
    {
        lv_obj_t *screen = lv_scr_act();
        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        lv_obj_t *label = lv_label_create(screen);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_label_set_text(label, "System Ready...");
        lv_obj_center(label);
        lvgl_port_unlock();
    }
    return err;
}

/**
 * @brief Main entry point to initialize the entire display subsystem.
 * @return
 * - ESP_OK: All display components initialized.
 * - Others: Error code from the failing sub-component.
 */
esp_err_t tool_display_init()
{
    esp_err_t err = ESP_OK;
    const gpio_config_t gpio_cofig = {
        .pin_bit_mask = (1ULL << BSP_LCD_TOUCH_POWER_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&gpio_cofig);
    if (err != ESP_OK)
        return err;
    gpio_set_level(BSP_LCD_TOUCH_POWER_EN, 1);

    err = bsp_i2c_init();
    if (err != ESP_OK)
        return err;
    err = display_port_init();
    if (err != ESP_OK)
        return err;
#if BSP_TOUCH_ENABLED
    err = touch_init();
    if (err != ESP_OK)
        return err;
#endif
    err = lvgl_init();
    DISPLAY_INFO("LVGL init success");
    if (err != ESP_OK)
        return err;
    vTaskDelay(pdMS_TO_TICKS(500));
    err = set_lcd_brightness_percentage(100);
    if (err != ESP_OK)
        return err;
    return err;
}

/**
 * @brief Agent Tool: Display text at specific coordinates
 * @param input_json JSON string, e.g., {"x": 100, "y": 200, "content": "Hello"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_display_text_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *x_obj = cJSON_GetObjectItem(root, "x");
    cJSON *y_obj = cJSON_GetObjectItem(root, "y");
    cJSON *content_obj = cJSON_GetObjectItem(root, "content");

    if (!cJSON_IsNumber(x_obj) || !cJSON_IsNumber(y_obj) || !cJSON_IsString(content_obj))
    {
        snprintf(output, output_size, "Error: Missing x, y, or content");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    int x = clamp_i16(x_obj->valueint, 0, BSP_H_SIZE-1);
    int y = clamp_i16(y_obj->valueint, 0, BSP_V_SIZE-1);
    const char *content = content_obj->valuestring;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);

        lv_obj_set_width(label, BSP_H_SIZE - x);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

        lv_label_set_text(label, content);
        lv_obj_set_pos(label, x, y);
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: Displayed text at (%d, %d)", x, y);
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Agent Tool: Draw multiple pixel-like points on the canvas
 * @param input_json JSON string, e.g., {"points": [{"x": 100, "y": 100}, {"x": 110, "y": 110}], "color": "blue"}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_draw_points_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *pts_arr = cJSON_GetObjectItem(root, "points");
    if (!cJSON_IsArray(pts_arr))
    {
        snprintf(output, output_size, "Error: 'points' array required.");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *color_obj = cJSON_GetObjectItem(root, "color");
    const char *color_str = cJSON_IsString(color_obj) ? color_obj->valuestring : "white";
    lv_color_t pt_color = get_lv_color_from_str(color_str);

    if (lvgl_port_lock(0))
    {
        if (global_canvas == NULL)
        {
            size_t buf_size = BSP_H_SIZE * BSP_V_SIZE * sizeof(lv_color_t);
            canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
            if (!canvas_buf)
            {
                lvgl_port_unlock();
                cJSON_Delete(root);
                snprintf(output, output_size, "Error: PSRAM alloc failed for points canvas.");
                return ESP_ERR_NO_MEM;
            }
            global_canvas = lv_canvas_create(lv_scr_act());
            lv_canvas_set_buffer(global_canvas, canvas_buf, BSP_H_SIZE, BSP_V_SIZE, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_fill_bg(global_canvas, lv_color_black(), LV_OPA_TRANSP);
            lv_obj_move_background(global_canvas);
        }

        lv_draw_rect_dsc_t pt_dsc;
        lv_draw_rect_dsc_init(&pt_dsc);
        pt_dsc.bg_color = pt_color;
        pt_dsc.bg_opa = LV_OPA_COVER;
        pt_dsc.border_width = 0;

        int pt_cnt = cJSON_GetArraySize(pts_arr);
        for (int i = 0; i < pt_cnt; i++)
        {
            cJSON *p = cJSON_GetArrayItem(pts_arr, i);
            cJSON *x_obj = cJSON_GetObjectItem(p, "x");
            cJSON *y_obj = cJSON_GetObjectItem(p, "y");

            if (cJSON_IsNumber(x_obj) && cJSON_IsNumber(y_obj))
            {
                int x = clamp_i16(x_obj->valueint, 0, 798);
                int y = clamp_i16(y_obj->valueint, 0, 478);

                lv_canvas_draw_rect(global_canvas, x, y, 2, 2, &pt_dsc);
            }
        }
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: %d points drawn in %s", pt_cnt, color_str);
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy.");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Agent Tool: Draws multiple shapes (lines or polygons) with filling on a Canvas.
 * @param input_json JSON string containing "shapes" array.
 * @param output Buffer for status message.
 * @param output_size Size of the output buffer.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t tool_draw_shapes_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *shapes_arr = cJSON_GetObjectItem(root, "shapes");
    if (!cJSON_IsArray(shapes_arr))
    {
        snprintf(output, output_size, "Error: 'shapes' array required.");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    if (lvgl_port_lock(0))
    {
        if (global_canvas == NULL)
        {
            canvas_buf = (lv_color_t *)heap_caps_malloc(BSP_H_SIZE * BSP_V_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
            if (!canvas_buf)
            {
                snprintf(output, output_size, " Error: PSRAM alloc failed. Check if PSRAM is enabled.");
                lvgl_port_unlock();
                cJSON_Delete(root);
                return ESP_ERR_NO_MEM;
            }
            global_canvas = lv_canvas_create(lv_scr_act());
            lv_canvas_set_buffer(global_canvas, canvas_buf, BSP_H_SIZE, BSP_V_SIZE, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_fill_bg(global_canvas, lv_color_black(), LV_OPA_TRANSP);
            lv_obj_move_background(global_canvas);
        }

        cJSON *shape_item;
        cJSON_ArrayForEach(shape_item, shapes_arr)
        {
            cJSON *pts_arr = cJSON_GetObjectItem(shape_item, "points");
            if (!cJSON_IsArray(pts_arr))
            {
                continue;
            }
            int pt_cnt = cJSON_GetArraySize(pts_arr);
            if (pt_cnt < 2)
            {
                continue;
            }
            lv_point_t *pts = (lv_point_t *)lv_mem_alloc(sizeof(lv_point_t) * pt_cnt);
            if (!pts)
            {
                continue;
            }

            bool parse_success = true;
            for (int i = 0; i < pt_cnt; i++)
            {
                cJSON *p = cJSON_GetArrayItem(pts_arr, i);
                if (!cJSON_IsObject(p))
                {
                    parse_success = false;
                    break;
                }

                cJSON *x_obj = cJSON_GetObjectItem(p, "x");
                cJSON *y_obj = cJSON_GetObjectItem(p, "y");
                if (!cJSON_IsNumber(x_obj) || !cJSON_IsNumber(y_obj))
                {
                    parse_success = false;
                    break;
                }

                pts[i].x = clamp_i16(x_obj->valueint, 0, BSP_H_SIZE-1);
                pts[i].y = clamp_i16(y_obj->valueint, 0, BSP_V_SIZE-1);
            }

            if (!parse_success)
            {
                lv_mem_free(pts);
                continue;
            }

            cJSON *fill_obj = cJSON_GetObjectItem(shape_item, "fill_color");
            cJSON *line_obj = cJSON_GetObjectItem(shape_item, "outline_color");
            bool is_closed = (pts[0].x == pts[pt_cnt - 1].x && pts[0].y == pts[pt_cnt - 1].y);
            const char *outline_str = cJSON_IsString(line_obj) ? line_obj->valuestring : "white";
            lv_color_t color_outline = get_lv_color_from_str(outline_str);

            if (pt_cnt >= 3 && is_closed && cJSON_IsString(fill_obj))
            {
                lv_draw_rect_dsc_t fill_dsc;
                lv_draw_rect_dsc_init(&fill_dsc);
                fill_dsc.bg_color = get_lv_color_from_str(fill_obj->valuestring);
                fill_dsc.bg_opa = LV_OPA_COVER;
                fill_dsc.border_width = 0;
                lv_canvas_draw_polygon(global_canvas, pts, pt_cnt, &fill_dsc);
            }

            lv_draw_line_dsc_t stroke_dsc;
            lv_draw_line_dsc_init(&stroke_dsc);
            stroke_dsc.color = color_outline;
            stroke_dsc.width = 2;
            stroke_dsc.round_start = true;
            stroke_dsc.round_end = true;
            for (int i = 0; i < pt_cnt - 1; i++)
            {
                lv_point_t segment[2];
                segment[0] = pts[i];
                segment[1] = pts[i + 1];
                lv_canvas_draw_line(global_canvas, segment, 2, &stroke_dsc);
            }

            lv_mem_free(pts);
        }
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: Shapes drawn on canvas.");
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy.");
    }
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Agent Tool: Clears all objects and resets the (BSP_H_SIZE)x(BSP_V_SIZE) LCD screen
 * @param input_json JSON Data
 * @param output  Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_clear_screen_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root)
    {
        snprintf(output, output_size, "Error: Invalid JSON format");
        return ESP_ERR_INVALID_ARG;
    }

    if (lvgl_port_lock(0))
    {
        lv_obj_clean(lv_scr_act());
        global_canvas = NULL;
        if (canvas_buf != NULL)
        {
            free(canvas_buf);
            canvas_buf = NULL;
        }
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
        lvgl_port_unlock();
        snprintf(output, output_size, "Success: Screen cleared, canvas reset.");
    }
    else
    {
        snprintf(output, output_size, "Error: LVGL system busy.");
    }

    cJSON_Delete(root);
    return ESP_OK;
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/