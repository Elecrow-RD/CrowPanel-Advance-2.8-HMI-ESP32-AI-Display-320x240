# 2.4-inch LVGL 9.1 PlatformIO Upgrade

## Result

This project is migrated from LVGL 8.3.11 to LVGL 9.1.0 and builds with PlatformIO Core 6.1.19, pioarduino Espressif32 55.03.38, and Arduino-ESP32 Core 3.3.8.

The original project is not modified. Third-party library source is not patched; dependencies are resolved from the official PlatformIO registry.

## Build

From the project directory:

```text
pio run
pio run -t upload
pio device monitor -b 115200
```

The verified environment is `esp32-s3-devkitc-1`. The current machine produced:

- RAM: 91,212 / 327,680 bytes (27.8%)
- Flash: 762,723 / 3,145,728 bytes (24.2%)
- Artifact: `../../pio_build/2.4inch_LVGL_9.1/esp32-s3-devkitc-1/firmware.bin`

`platformio.ini` places downloaded libraries under `%USERPROFILE%/.platformio/libdeps/2.4inch_LVGL_9.1` and build output under `../../pio_build/2.4inch_LVGL_9.1`. The external build directory avoids a Windows Xtensa linker failure when the project lives under a path containing Chinese characters.

## Library versions

| Library | Previous | Selected | Action |
| --- | --- | --- | --- |
| LVGL | 8.3.11 | 9.1.0 | Required API migration |
| LovyanGFX | 1.2.19 | 1.2.25 | Official registry upgrade |
| Adafruit SSD1306 | 2.5.13 | Not used | Removed unused dependency |
| Adafruit GFX | Transitive | Not used | Removed unused dependency |
| TAMC GT911 | 1.0.2 | Not used | Removed duplicate, unused input path |
| RAK14014-FT6336U | 1.0.1 | Not used | Removed unused dependency |

## LVGL migration

- `lv_disp_draw_buf_t` and `lv_disp_drv_t` were replaced with `lv_display_t` and `lv_display_create()`.
- Display setup now uses `lv_display_set_color_format()`, `lv_display_set_flush_cb()`, and `lv_display_set_buffers()`.
- `lv_disp_flush_ready()` became `lv_display_flush_ready()`.
- Input setup now uses `lv_indev_create()`, `lv_indev_set_type()`, and `lv_indev_set_read_cb()`.
- `lv_tick_set_cb(millis)` supplies the LVGL 9 tick source.
- The SquareLine-generated UI already targets LVGL 9.1 and compiles without compatibility shims.
- Partial DMA buffers use 40 scan lines each, avoiding a full-screen PSRAM allocation while keeping refresh responsive on the 2.4-inch panel.

## Hardware notes

The display driver configures an ST7789 panel at 320x240 with LovyanGFX `Touch_FT5x06` on I2C address `0x38`. The legacy `include/touch.h` describes GT911, but the original LVGL input callback used `gfx.getTouch()` instead of that separate driver. The current code keeps the effective callback path and removes the duplicate TAMC dependency. Confirm the physical touch controller before flashing production hardware.

`Lamp_on()` and `Lamp_off()` now drive GPIO 18 through the project lamp-control interface. ON writes HIGH and OFF writes LOW, matching the original LOW startup state.

No physical board, display, touch panel, or serial upload was available in this environment. Build and link are verified; runtime display orientation, touch calibration, and lamp I/O still require hardware validation.

## Files changed

- `platformio.ini`: pioarduino platform, Arduino-ESP32 3.3.8 package, LVGL configuration flags, 16 MB flash settings, partition naming, and short dependency/build paths.
- `include/lv_conf.h`: project LVGL 9.1 configuration.
- `src/main.cpp`: LVGL 9 display/input lifecycle, tick source, DMA buffers, flush completion, and coordinate bounds checking.
- `.gitignore`: generated PlatformIO and editor artifacts.

The UI files and image/font assets are retained; no third-party library source is stored in this project.
