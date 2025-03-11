1, Product picture

![CrowPanel Advance 2.8”HMI AI Display for Meshtastic](https://www.elecrow.com/media/catalog/product/cache/9e67447b006ee4d9559353b91d12add5/e/s/esp32_advance_hmi_02_dis01728a.jpg)

2, Product version number

|      | Hardware | Software | Remark |
| ---- | -------- | -------- | ------ |
| 1    | V1.0     | V1.0     | latest |

3, product information

| **Main Chip-**ESP32-S3-WROOM-1-N16R8         |                                                              |
| -------------------------------------------- | ------------------------------------------------------------ |
| CPU/SoC                                      | high-performance Xtensa 32-bit LX7 dual-core processor, with up to 240MHz |
| System Memory                                | 512KB SRAM、8M PSRAM                                         |
| Memory                                       | 16M Flash，384KB ROM                                         |
| Development Language                         | MicroPython、C/C++                                           |
| Development Environment                      | ESP-IDF、Arduino IDE、LVGL、PlatformIO、Micro Python         |
| **Screen**                                   |                                                              |
| Size                                         | 2.8 inch                                                     |
| Diver IC                                     | ST7789                                                       |
| Resolution                                   | 320*240                                                      |
| Display Panel                                | IPS Panel                                                    |
| Touch Panel                                  | Capacitive Single Touch                                      |
| Viewing Angle                                | 178°                                                         |
| Brightness                                   | 400 cd/m²(Typ.)                                              |
| Color Depth                                  | 16-bit                                                       |
| **Wireless Communication - Onboard Antenna** |                                                              |
| WiFi                                         | Support 2.4GHz, 802.11a/b/g/n                                |
| Bluetooth                                    | Support Bluetooth 5.0 and BLE                                |
| Other                                        | **LoRa**                                                     |
| **Interface/Function**                       |                                                              |
| Interface                                    | USB port, UART, I2C, SD card slot, battery socket, speaker port, microphone, etc. |
| Function                                     | RTC clock, audio amplifier, volume control, battery charge management, USB to UART, etc. |
| **Button/LED Indicator**                     |                                                              |
| Reset Button                                 | Yes, press to reset device                                   |
| Boot Button                                  | Yes, press and hold the power button to burn the program     |
| PWR                                          | Power indicator                                              |
| CHG                                          | Lithium battery charging status, completion indication       |
| **Other**                                    |                                                              |
| Installation method                          | Back hanging, fixed hole                                     |
| Operating temperature                        | -20~70 °C                                                    |
| Storage temperature                          | -30~80 °C                                                    |
| Power Input                                  | 5V/2A, USB or UART terminal                                  |
| Dimensions                                   | 85.4*58.5*15.5*mm                                            |

4, Use the driver module

| Name   | dependency library                      |
| ------ | --------------------------------------- |
| LVGL   | lvgl/lvgl@8.3.3                         |
| ST7789 | Adafruit GFX Library<br/>version=1.11.0 |

5,Quick Start



6,Folder structure.

|--3D file： Contains 3D model files (.stp) for the hardware. These files can be used for visualization, enclosure design, or integration into CAD software.

|--Datasheet: Includes datasheets for components used in the project, providing detailed specifications, electrical characteristics, and pin configurations.

|--Eagle_SCH&PCB: Contains **Eagle CAD** schematic (`.sch`) and PCB layout (`.brd`) files. These are used for circuit design and PCB manufacturing.

|--example: Provides example code and projects to demonstrate how to use the hardware and libraries. These examples help users get started quickly.

|--factory_firmware: Stores pre-compiled factory firmware that can be directly flashed onto the device. This ensures the device runs the default functionality.

|--factory_sourcecode:  Contains the source code for the factory firmware, allowing users to modify and rebuild the firmware as needed.

|--libraries: Includes necessary libraries required for compiling and running the project. These libraries provide drivers and additional functionalities for the hardware.

7,Pin definition

#define SD_MOSI 6
#define SD_MISO 4
#define SD_SCK 5
#define SD_CS 7 



cfg.pin_sclk = 42;  // SPI、SCLK

cfg.pin_mosi = 39;  // SPI、CLK

cfg.pin_miso = -1;  // SPI、MISO

cfg.pin_dc = 41; 

#define TOUCH_GT911_SCL 16
#define TOUCH_GT911_SDA 15
