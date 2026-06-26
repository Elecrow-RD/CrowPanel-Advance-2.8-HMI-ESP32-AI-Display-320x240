# #Add sources from ui directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")

idf_component_register(
    SRCS 
        "main.c"
        "./setup/setup_gpio.c"
        "./setup/setup_spi.c"
        "./setup/setup_i2c.c"
        "./device/device_lcd.c"
        "./device/device_touch.c"
        "./device/FT6336U.c"
        "./soft_drv/soft_drv_lvgl_port.c"
        "./lv_ui/lv_ui.c"
        ${SRC_UI}
    INCLUDE_DIRS
        "."
        "./setup/include"
        "./device/include"
        "./soft_drv/include"
        "./lv_ui/include"
        "./ui"
)