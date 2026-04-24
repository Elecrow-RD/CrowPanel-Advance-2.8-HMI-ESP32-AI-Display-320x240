#include "tool_registry.h"
#include "mimi_config.h"
#include "tools/tool_web_search.h"
#include "tools/tool_get_time.h"
#include "tools/tool_files.h"
#include "tools/tool_cron.h"
#include "tools/tool_gpio.h"
#include "tools/tool_led.h"
#include "tools/tool_display.h"
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "tools";

#define MAX_TOOLS 20

static mimi_tool_t s_tools[MAX_TOOLS];
static int s_tool_count = 0;
static char *s_tools_json = NULL;  /* cached JSON array string */

static void register_tool(const mimi_tool_t *tool)
{
    if (s_tool_count >= MAX_TOOLS) {
        ESP_LOGE(TAG, "Tool registry full");
        return;
    }
    s_tools[s_tool_count++] = *tool;
    ESP_LOGI(TAG, "Registered tool: %s", tool->name);
}

static void build_tools_json(void)
{
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < s_tool_count; i++) {
        cJSON *tool = cJSON_CreateObject();
        cJSON_AddStringToObject(tool, "name", s_tools[i].name);
        cJSON_AddStringToObject(tool, "description", s_tools[i].description);

        cJSON *schema = cJSON_Parse(s_tools[i].input_schema_json);
        if (schema) {
            cJSON_AddItemToObject(tool, "input_schema", schema);
        }

        cJSON_AddItemToArray(arr, tool);
    }

    free(s_tools_json);
    s_tools_json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    ESP_LOGI(TAG, "Tools JSON built (%d tools)", s_tool_count);
}

esp_err_t tool_registry_init(void)
{
    s_tool_count = 0;

    /* Register web_search */
    tool_web_search_init();

    mimi_tool_t ws = {
        .name = "web_search",
        .description = "Search the web for current information via Tavily (preferred) or Brave when configured.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"query\":{\"type\":\"string\",\"description\":\"The search query\"}},"
            "\"required\":[\"query\"]}",
        .execute = tool_web_search_execute,
    };
    register_tool(&ws);

    /* Register get_current_time */
    mimi_tool_t gt = {
        .name = "get_current_time",
        .description = "Get the current date and time. Also sets the system clock. Call this when you need to know what time or date it is.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_get_time_execute,
    };
    register_tool(&gt);

    /* Register read_file */
    mimi_tool_t rf = {
        .name = "read_file",
        .description = "Read a file from SPIFFS storage. Path must start with " MIMI_SPIFFS_BASE "/.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_SPIFFS_BASE "/\"}},"
            "\"required\":[\"path\"]}",
        .execute = tool_read_file_execute,
    };
    register_tool(&rf);

    /* Register write_file */
    mimi_tool_t wf = {
        .name = "write_file",
        .description = "Write or overwrite a file on SPIFFS storage. Path must start with " MIMI_SPIFFS_BASE "/.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_SPIFFS_BASE "/\"},"
            "\"content\":{\"type\":\"string\",\"description\":\"File content to write\"}},"
            "\"required\":[\"path\",\"content\"]}",
        .execute = tool_write_file_execute,
    };
    register_tool(&wf);

    /* Register edit_file */
    mimi_tool_t ef = {
        .name = "edit_file",
        .description = "Find and replace text in a file on SPIFFS. Replaces first occurrence of old_string with new_string.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_SPIFFS_BASE "/\"},"
            "\"old_string\":{\"type\":\"string\",\"description\":\"Text to find\"},"
            "\"new_string\":{\"type\":\"string\",\"description\":\"Replacement text\"}},"
            "\"required\":[\"path\",\"old_string\",\"new_string\"]}",
        .execute = tool_edit_file_execute,
    };
    register_tool(&ef);

    /* Register list_dir */
    mimi_tool_t ld = {
        .name = "list_dir",
        .description = "List files on SPIFFS storage, optionally filtered by path prefix.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"prefix\":{\"type\":\"string\",\"description\":\"Optional path prefix filter, e.g. " MIMI_SPIFFS_BASE "/memory/\"}},"
            "\"required\":[]}",
        .execute = tool_list_dir_execute,
    };
    register_tool(&ld);

    /* Register cron_add */
    mimi_tool_t ca = {
        .name = "cron_add",
        .description = "Schedule a recurring or one-shot task. The message will trigger an agent turn when the job fires.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{"
            "\"name\":{\"type\":\"string\",\"description\":\"Short name for the job\"},"
            "\"schedule_type\":{\"type\":\"string\",\"description\":\"'every' for recurring interval or 'at' for one-shot at a unix timestamp\"},"
            "\"interval_s\":{\"type\":\"integer\",\"description\":\"Interval in seconds (required for 'every')\"},"
            "\"at_epoch\":{\"type\":\"integer\",\"description\":\"Unix timestamp to fire at (required for 'at')\"},"
            "\"message\":{\"type\":\"string\",\"description\":\"Message to inject when the job fires, triggering an agent turn\"},"
            "\"channel\":{\"type\":\"string\",\"description\":\"Optional reply channel (e.g. 'telegram'). If omitted, current turn channel is used when available\"},"
            "\"chat_id\":{\"type\":\"string\",\"description\":\"Optional reply chat_id. Required when channel='telegram'. If omitted during a Telegram turn, current chat_id is used\"}"
            "},"
            "\"required\":[\"name\",\"schedule_type\",\"message\"]}",
        .execute = tool_cron_add_execute,
    };
    register_tool(&ca);

    /* Register cron_list */
    mimi_tool_t cl = {
        .name = "cron_list",
        .description = "List all scheduled cron jobs with their status, schedule, and IDs.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_cron_list_execute,
    };
    register_tool(&cl);

    /* Register cron_remove */
    mimi_tool_t cr = {
        .name = "cron_remove",
        .description = "Remove a scheduled cron job by its ID.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"job_id\":{\"type\":\"string\",\"description\":\"The 8-character job ID to remove\"}},"
            "\"required\":[\"job_id\"]}",
        .execute = tool_cron_remove_execute,
    };
    register_tool(&cr);

    /* Register GPIO tools */
    tool_gpio_init();

    mimi_tool_t gw = {
        .name = "gpio_write",
        .description = "Set a GPIO pin HIGH or LOW. Controls LEDs, relays, and other digital outputs.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"pin\":{\"type\":\"integer\",\"description\":\"GPIO pin number\"},"
            "\"state\":{\"type\":\"integer\",\"description\":\"1 for HIGH, 0 for LOW\"}},"
            "\"required\":[\"pin\",\"state\"]}",
        .execute = tool_gpio_write_execute,
    };
    register_tool(&gw);

    mimi_tool_t gr = {
        .name = "gpio_read",
        .description = "Read a GPIO pin state. Returns HIGH or LOW. Use for checking switches, sensors, and digital inputs.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"pin\":{\"type\":\"integer\",\"description\":\"GPIO pin number\"}},"
            "\"required\":[\"pin\"]}",
        .execute = tool_gpio_read_execute,
    };
    register_tool(&gr);

    mimi_tool_t ga = {
        .name = "gpio_read_all",
        .description = "Read all allowed GPIO pin states in a single call. Returns each pin's HIGH/LOW state.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_gpio_read_all_execute,
    };
    register_tool(&ga);

    if (tool_led_init() != ESP_OK)
        ESP_LOGE(TAG, "Failed to initialize LED hardware/task");
    else
    {
        mimi_tool_t lc = {
            .name = "led_control",
            .description = "Control the system visual indicator LED. "
                           "USE THIS TOOL for user-facing feedback like 'turn on the light', 'make it blink', or 'status alerts'. "
                           "DO NOT use raw GPIO tools for LED status. "
                           "Actions: 'on' (steady), 'off' (extinguish), 'blink' (flashing). "
                           "Default: 500ms interval, 3000ms duration.",
            .input_schema_json =
                "{"
                "\"type\":\"object\","
                "\"properties\":{"
                "\"action\":{"
                "\"type\":\"string\","
                "\"enum\":[\"on\",\"off\",\"blink\"],"
                "\"description\":\"REQUIRED. The operation mode for the LED.\""
                "},"
                "\"interval_ms\":{"
                "\"type\":\"integer\","
                "\"minimum\":10,"
                "\"default\":500,"
                "\"description\":\"Blink interval in milliseconds (speed).\""
                "},"
                "\"duration_ms\":{"
                "\"type\":\"integer\","
                "\"default\":3000,"
                "\"description\":\"Total time to keep the state in ms. Use -1 for infinite.\""
                "}"
                "},"
                "\"required\":[\"action\"]"
                "}",
            .execute = tool_led_control_execute,
        };
        register_tool(&lc);
    }

    if (tool_display_init() != ESP_OK)
        ESP_LOGE(TAG, "Failed to initialize display hardware/task");
    else
    {
        mimi_tool_t dt = {
            .name = "display_draw_text",
            .description = "Render text at specific coordinates on the 320x240 screen. "
                           "The text will automatically wrap at the right screen boundary (320px). "
                           "Coordinates (0,0) is the top-left corner. Default color is white.",
            .input_schema_json =
                "{"
                "\"type\":\"object\","
                "\"properties\":{"
                "\"x\":{\"type\":\"integer\",\"description\":\"Start X (0-320)\"},"
                "\"y\":{\"type\":\"integer\",\"description\":\"Start Y (0-240)\"},"
                "\"content\":{\"type\":\"string\",\"description\":\"Text to display. Will auto-wrap at screen boundary.\"}"
                "},"
                "\"required\":[\"x\",\"y\",\"content\"]"
                "}",
            .execute = tool_display_text_execute,
        };
        register_tool(&dt);

        // mimi_tool_t dp = {
        //     .name = "display_draw_pixel",
        //     .description = "Draw a single 2x2 pixel point at (x, y) on the 320x240 screen. "
        //                    "calling it multiple times with calculated coordinates. "
        //                    "Useful for fine details or star-like effects.",
        //     .input_schema_json =
        //         "{"
        //         "\"type\":\"object\","
        //         "\"properties\":{"
        //         "\"x\":{\"type\":\"integer\",\"description\":\"X coordinate (0-320)\"},"
        //         "\"y\":{\"type\":\"integer\",\"description\":\"Y coordinate (0-240)\"}"
        //         "},"
        //         "\"required\":[\"x\",\"y\"]"
        //         "}",
        //     .execute = tool_draw_pixel_execute,
        // };

        mimi_tool_t dp = {
            .name = "draw_points",
            .description = "Draw multiple dots/pixels in a single call on the 320x240 screen. "
                           "More efficient than draw_shapes for fine details like stars, snow, or textures. "
                           "Supports an array of coordinates and a single color for the entire batch.",
            .input_schema_json = "{"
                                 "\"type\":\"object\","
                                 "\"properties\":{"
                                 "\"points\":{"
                                 "\"type\":\"array\","
                                 "\"description\":\"List of (x, y) coordinates for each dot.\","
                                 "\"items\":{"
                                 "\"type\":\"object\","
                                 "\"properties\":{"
                                 "\"x\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":320},"
                                 "\"y\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":240}"
                                 "},"
                                 "\"required\":[\"x\",\"y\"]"
                                 "}"
                                 "},"
                                 "\"color\":{"
                                 "\"type\":\"string\","
                                 "\"description\":\"Color for all points in this batch.\","
                                 "\"enum\":[\"white\",\"red\",\"green\",\"blue\",\"yellow\",\"black\",\"pink\"]"
                                 "}"
                                 "},"
                                 "\"required\":[\"points\"]"
                                 "}",
            .execute = tool_draw_points_execute,
        };
        register_tool(&dp);

        mimi_tool_t ds = {
            .name = "draw_shapes",
            .description = "Draw shapes on 320x240 LCD. "
                           "If the first and last points are identical, the shape is considered CLOSED and will be filled with 'fill_color'. "
                           "If points are NOT identical, it is an OPEN polyline and only 'outline_color' will be visible. "
                           "Each shape in 'shapes' array is processed independently. "
                           "Send all parts of a complex drawing (e.g., a character) in a single call for efficiency.",
            .input_schema_json =
                "{"
                "\"type\":\"object\","
                "\"properties\":{"
                "\"shapes\":{"
                "\"type\":\"array\","
                "\"description\":\"Array of shapes. Close the points loop to fill.\","
                "\"items\":{"
                "\"type\":\"object\","
                "\"properties\":{"
                "\"points\":{"
                "\"type\":\"array\","
                "\"items\":{"
                "\"type\":\"object\","
                "\"properties\":{"
                "\"x\":{\"type\":\"integer\"},"
                "\"y\":{\"type\":\"integer\"}"
                "},"
                "\"required\":[\"x\",\"y\"]"
                "}"
                "},"
                "\"outline_color\":{\"type\":\"string\",\"enum\":[\"white\",\"red\",\"green\",\"blue\",\"yellow\",\"black\",\"pink\"]},"
                "\"fill_color\":{\"type\":\"string\",\"enum\":[\"white\",\"red\",\"green\",\"blue\",\"yellow\",\"black\",\"pink\"]}"
                "},"
                "\"required\":[\"points\"]"
                "}"
                "}"
                "},"
                "\"required\":[\"shapes\"]"
                "}",
            .execute = tool_draw_shapes_execute,
        };
        register_tool(&ds);

        mimi_tool_t cs = {
            .name = "clear_screen",
            .description = "Wipe the 320x240 LCD clean and reset to black background. "
                           "CRITICAL: Always call this ONCE at the start of a new drawing session "
                           "to free memory and prevent overlapping with old visuals.",
            .input_schema_json = "{\"type\":\"object\","
                                 "\"properties\":{},"
                                 "\"required\":[]}",
            .execute = tool_clear_screen_execute,
        };
        register_tool(&cs);
    }

    build_tools_json();

    ESP_LOGI(TAG, "Tool registry initialized");
    return ESP_OK;
}

const char *tool_registry_get_tools_json(void)
{
    return s_tools_json;
}

esp_err_t tool_registry_execute(const char *name, const char *input_json,
                                char *output, size_t output_size)
{
    for (int i = 0; i < s_tool_count; i++) {
        if (strcmp(s_tools[i].name, name) == 0) {
            ESP_LOGI(TAG, "Executing tool: %s", name);
            return s_tools[i].execute(input_json, output, output_size);
        }
    }

    ESP_LOGW(TAG, "Unknown tool: %s", name);
    snprintf(output, output_size, "Error: unknown tool '%s'", name);
    return ESP_ERR_NOT_FOUND;
}
