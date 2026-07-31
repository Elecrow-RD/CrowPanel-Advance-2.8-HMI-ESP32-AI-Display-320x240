# ESP32 2.8-Inch Hardware Driver Guide

| Item | Details |
|---|---|
| Document Version | V1.0 |
| Applicable Hardware | ESP32 Display 2.8 inch V1.2 |
| Date | 2026-07-30 |
| Author | OpenAI Codex (compiled from the project schematic and verified examples) |
| Document Purpose | Hardware maintenance, driver porting, production testing, and onboarding handoff |

## 1. Document Scope and Evaluation Criteria

This guide cross-checks `1.2/ESP32 Display 2.8 inch V1.2(1).sch`, the PDF schematic of the same version, and the working Arduino examples in `2.4_2.8_Arduino/lesson-*`. The schematic version is V1.2, and the PDF was generated on 2025-07-18.

Information reliability is evaluated in the following order:

1. **Functional code in successfully tested examples**: Highest priority as the actual driver configuration.
2. **Eagle schematic netlist and visual annotations in the PDF**: Used to verify electrical connections, peripheral circuits, and components not covered by code.
3. **Commented-out code, third-party library examples, and generic macros**: Treated only as candidate information, not as configurations verified for this board.

Status definitions used in this document:

- **Verified**: The project contains functional driver calls specifically for this board.
- **Hardware Confirmed**: The connection can be confirmed from the schematic, but the repository does not contain an independently verified example.
- **Extension Verified**: Configuration is provided for an external module; the module is not a permanently soldered component on the main board.
- **To Be Confirmed**: The code conflicts with the schematic and must be retested against the physical hardware version.

> Important principle: When the code conflicts with the schematic, this guide records the runtime parameters from functional code while retaining the schematic values and documenting the discrepancy risk. Do not modify production hardware based solely on this information; verify the physical PCB version first.

## 2. Hardware Architecture and Peripheral Overview

| Category | Device/Interface | Primary Connections | Driver Method | Status |
|---|---|---|---|---|
| MCU | ESP32-S3-WROOM-1 | All board GPIOs; USB GPIO19/20 | Arduino-ESP32 / underlying ESP-IDF | Verified |
| Display | 2.8-inch TFT, configured as ST7789 in code | SCK 42, MOSI 39, DC 41, CS 40, backlight 38 | SPI mode 0 + DMA | Verified |
| Touch | Capacitive touch, using GT911 in code | SDA 15, SCL 16; schematic INT 47, RST 48 | I2C, polled reading | Verified with discrepancies |
| Storage | microSD card socket | MISO 4, SCK 5, MOSI 6, CS 7 | SPI + Arduino FS/SD | Verified |
| Audio Output | NS4168 mono Class-D amplifier + speaker connector | LRCLK 11, SDATA 12, BCLK 13, CTRL 21 | I2S + GPIO enable | Verified |
| Digital Microphone | LMD3526B261 PDM/I2S MIC | CLK/data switched to GPIO9/10 through SGM3799 | PDM/I2S input | Hardware Confirmed |
| Buzzer | BEEP_5025 + NPN driver | GPIO8 | GPIO/PWM, external transistor driver | Hardware Confirmed |
| Wireless Extension | nRF24L01 | SPI 10/9/3, CE 1, CSN 2, select 45 | SPI, RF24 library | Extension Verified |
| Wireless Extension | SX1262 LoRa/LoRaWAN | SPI 10/9/3, NSS 0, DIO1 1, RST 2, BUSY 46 | SPI, RadioLib | Extension Verified |
| Wireless Extension | UART Zigbee/other module | RX 2, TX 1, select 45 | UART 115200 8N1 | Extension Verified |
| USB Debugging | USB-C + CH340K | UART0; USB-to-UART automatic download circuit | UART/USB CDC | Verified (serial port) |
| USB OTG | Separate USB-C | GPIO19 D-, GPIO20 D+ | ESP32-S3 USB OTG | Hardware Confirmed |
| Buttons | BOOT, RESET | GPIO0, CH_EN | Active-low buttons | Hardware Confirmed |
| Battery Charging | TP4059 + STC8G1K08 indicator control | VBAT, CHRG, DONE | Independent analog power circuit/auxiliary MCU | Hardware Confirmed |
| Main Power Supply | RY3420 buck-boost power stage, MOS load switches | VIN, 3V3, 3V3_OUT1/2 | Independent hardware, no main MCU initialization | Hardware Confirmed |
| Expansion Interfaces | 5V UART0, 3.3V UART1, 3.3V I2C, wireless adapter port | See Section 5 | UART/I2C/SPI/GPIO | Hardware Confirmed |

No independent environmental, inertial, or optical sensors were found in this schematic revision. Do not mistake devices supported by third-party library directories for onboard sensors.

## 3. GPIO and Multiplexing Summary

| GPIO | Schematic Net/Default Function | Verified Code Usage | Electrical/Multiplexing Notes |
|---:|---|---|---|
| 0 | BOOT_BUSY | SX1262 NSS | Bootstrapping pin, active-low BOOT; boot risk when LoRa is used |
| 1 | UART2 TX | GT911 address sequencing output; nRF24 CE; SX1262 DIO1; UART extension TX | Mutually exclusive across use cases |
| 2 | UART2 RX | GT911 address sequencing output; nRF24 CSN; SX1262 RST; UART extension RX | Mutually exclusive across use cases |
| 3 | Wireless SPI MOSI | nRF24/SX1262 MOSI | Wireless path after SGM3799 |
| 4 | SD MISO | SD MISO | 10 kΩ pull-up |
| 5 | SD SCK | SD SCK | Dedicated card clock |
| 6 | SD MOSI | SD MOSI | 10 kΩ pull-up |
| 7 | SD CS | SD CS | 10 kΩ pull-up |
| 8 | BEEP | No dedicated example provided | Drives NPN through 1 kΩ; PWM is recommended for tone generation |
| 9 | SGM3799 COM2 | Wireless SPI MISO or microphone CLK | Selected by GPIO45; cannot be used simultaneously |
| 10 | SGM3799 COM3 | Wireless SPI SCK or microphone DATA | Selected by GPIO45; cannot be used simultaneously |
| 11 | I2S LRCLK | Amplifier LRCLK | I2S output |
| 12 | I2S SDIN | Amplifier SDATA | MCU data output, amplifier data input |
| 13 | I2S BCLK | Amplifier BCLK | I2S bit clock |
| 14 | TFT_PWR | Not enabled by examples | Optional display power control in schematic; component annotation includes NC |
| 15 | I2C SDA | Touch/expansion I2C SDA | Onboard 4.7 kΩ pull-up to 3.3 V |
| 16 | I2C SCL | Touch/expansion I2C SCL | Onboard 4.7 kΩ pull-up to 3.3 V |
| 17 | UART1 TX | No dedicated transmit/receive example provided | J6 3.3 V UART1 |
| 18 | UART1 RX | LVGL example only configures it as OUTPUT without setting a level | J6 3.3 V UART1; this initialization appears to be legacy code |
| 19 | USB D- | No OTG example provided | Native USB differential line; do not use as a general-purpose GPIO |
| 20 | USB D+ | No OTG example provided | Native USB differential line; do not use as a general-purpose GPIO |
| 21 | NS_CTRL | Amplifier enable, driven LOW by code | LOW is the currently verified operating state |
| 38 | LCD backlight | GPIO HIGH or 5 kHz/8-bit PWM | HIGH turns the backlight on; drives the backlight cathode through an NPN transistor |
| 39 | TFT SDA/MOSI | LCD MOSI | Write-only; MISO is disabled in code |
| 40 | TFT CS | LCD CS | Active-low chip select |
| 41 | TFT RS/DC | LCD D/C | Command/data selection |
| 42 | TFT SCK | LCD SCK | SPI2 clock |
| 45 | SPI switching | Driven LOW by wireless examples | Controls the SGM3799 analog switch, selecting between wireless SPI and the digital MIC |
| 46 | Wireless CS | nRF24 SPI SS; SX1262 BUSY | The actual nRF24 CSN is GPIO2; GPIO46 is still passed to SPI begin |
| 47 | TP_INT | Candidate INT in LovyanGFX; not used by the actual touch.h | Touch interrupt in the schematic; current driver uses polling |
| 48 | TP_RST | Not used by actual examples | Touch reset in the schematic |

## 4. Peripheral Driver Details

### 4.1 ESP32-S3-WROOM-1 MCU

- **Power and reset**: 3.3 V passes through a ferrite bead into `ESP_3V3`; `CH_EN` uses a 10 kΩ pull-up and a 1 µF capacitor to form the power-on reset circuit, and the RESET button pulls it low.
- **Boot**: The BOOT button pulls GPIO0 low to enter download mode. GPIO0 is also used as NSS by the LoRa example, so the extension driver must prevent the peripheral from pulling this pin low during reset.
- **Software layer**: Arduino-ESP32; the display driver also calls ESP-IDF `SPI2_HOST`, DMA, and heap capabilities. The LVGL example uses PSRAM to allocate two full-screen RGB565 buffers.
- **Serial logging**: Most examples use 115200 baud; the basic serial example in lesson-01 uses 9600 baud. Maintenance firmware should use a consistent serial baud rate to avoid incorrectly concluding that there is no output.
- **Wi-Fi**: Wi-Fi is integrated into the chip and is accessed through Arduino `WiFi`/`WiFiMulti` without external GPIOs; the online audio example uses STA mode.

Key initialization:

```cpp
Serial.begin(115200);
lv_color_t *buf = (lv_color_t *)heap_caps_malloc(
    sizeof(lv_color_t) * 320 * 240, MALLOC_CAP_SPIRAM);
```

### 4.2 TFT LCD (ST7789 Configuration)

| Parameter | Actual Configuration |
|---|---|
| Resolution | Physical driver space: 240 × 320; application landscape mode: 320 × 240 |
| Bus | ESP32-S3 `SPI2_HOST`, SPI mode 0 |
| Write/read clock | 80 MHz / 16 MHz; panel configured as non-readable |
| Pins | SCK 42, MOSI 39, MISO -1, DC 41, CS 40, RST -1 |
| DMA | `SPI_DMA_CH_AUTO`, transaction locking enabled |
| Panel parameters | `invert=true`, `rgb_order=false`, `offset_rotation=3` |
| Software layer | LovyanGFX; GUI examples use an LVGL 9-style API |

Initialization sequence: call `gfx.init()` first, followed by `gfx.initDMA()` and `gfx.startWrite()`; LVGL then registers RGB565 double buffering and the flush callback. The schematic includes a separate `TFT_RST` RC/diode reset network, so configuring the panel RST as `-1` in code is reasonable. The optional TFT power switch on GPIO14 is not enabled in verified examples and should not be pulled low arbitrarily during porting.

```cpp
cfg.spi_mode = 0;
cfg.freq_write = 80000000;
cfg.pin_sclk = 42;
cfg.pin_mosi = 39;
cfg.pin_dc = 41;
cfg.pin_cs = 40;
```

### 4.3 LCD Backlight

- **Connection**: GPIO38 drives an SS8050 NPN through 1 kΩ to control LCD `LEDK`; `LEDA` is connected to TFT 3.3 V.
- **Polarity**: Code verifies that GPIO38 HIGH turns the backlight on.
- **On/off control**: `pinMode(38, OUTPUT); digitalWrite(38, HIGH);`.
- **Dimming control**: Arduino-ESP32 LEDC, 5 kHz, 8-bit, duty cycle 0...255.
- **Note**: This is an external transistor-driven load, so the GPIO does not directly carry the backlight current. If the PWM frequency is changed, verify low-brightness flicker and EMI.

```cpp
ledcAttach(38, 5000, 8);
ledcWrite(38, duty);  // 0...255
```

### 4.4 Capacitive Touch

| Item | Schematic | Actual Functional Code |
|---|---|---|
| Controller | Chip model not specified at the FPC interface | `TAMC_GT911` |
| SDA/SCL | GPIO15/GPIO16 | GPIO15/GPIO16 |
| INT/RST | GPIO47/GPIO48 | `TOUCH_GT911_INT=-1`, `RST=-1`, polling; GPIO1/2 are additionally used for address sequencing |
| Address | Not specified | `0x5D` |
| Frequency | Not specified | LovyanGFX configured for 400 kHz; standalone `touch.h` does not explicitly call `Wire.setClock` |
| Coordinates | Touch FPC | Raw mapping: 480 × 320; application then rotates it to 320 × 240 |

The actual LVGL example starts I2C first, pulls GPIO1 and GPIO2 low, pulls GPIO2 high after 20 ms, waits 100 ms, releases GPIO1, and finally initializes the device at address `0x5D`. This sequence is inconsistent with the dedicated GPIO47/48 touch pins shown in the schematic. The most likely causes are prototype rewiring, an adapter-board connection, or legacy example code. The configuration must be confirmed against the specific physical hardware.

```cpp
Wire.begin(15, 16);
pinMode(1, OUTPUT); pinMode(2, OUTPUT);
digitalWrite(1, LOW); digitalWrite(2, LOW);
delay(20); digitalWrite(2, HIGH);
delay(100); pinMode(1, INPUT);
touch_init(0x5D);
```

Porting recommendation: Scan `0x5D` and `0x14` first. If neither responds, reset the controller through GPIO48 and configure its address/interrupt through GPIO47 according to the schematic. Do not run nRF24, LoRa, or the UART wireless extension simultaneously because they also use GPIO1/2.

### 4.5 microSD

- **Connection**: GPIO4 MISO, GPIO5 SCK, GPIO6 MOSI, GPIO7 CS; the data lines have 10 kΩ pull-ups and use a 3.3 V supply.
- **Bus**: A separate Arduino `SPIClass(HSPI)` object; SPI mode is managed by the SD library.
- **Mount parameters**: `SD.begin(7, SD_SPI, 80000000)`, requesting 80 MHz in code.
- **Software layer**: Arduino `SPI`, `FS`, and `SD`.
- **Initialization sequence**: `SD_SPI.begin(5, 4, 6)` → `SD.begin(...)` → query capacity/directories.
- **Risk**: Operation at 80 MHz is sensitive to the card, trace layout, and Arduino core version. If mounting fails intermittently, reduce the frequency to 20 MHz or 40 MHz for verification before changing any pins.

```cpp
SD_SPI.begin(5, 4, 6);
if (!SD.begin(7, SD_SPI, 80000000)) { /* fail */ }
```

### 4.6 Audio Output: NS4168 + Speaker

- **Interface**: GPIO13 BCLK, GPIO11 LRCLK, GPIO12 SDATA; the ESP32 operates as the I2S master output.
- **Enable**: GPIO21 is connected to the amplifier `CTRL` power-control circuit. Verified code drives it LOW before playback.
- **Output**: Differential `ROUT+`/`ROUT-` connects to the 2-pin speaker connector through ferrite beads. Neither terminal is ground; never short either terminal to GND.
- **Software layer**: The `Audio` class from `ESP32-audioI2S`, using Arduino-ESP32 I2S/Wi-Fi underneath.
- **Key parameters**: `audio.setPinout(13, 11, 12)`; volume 20 (library range is approximately 0...21); the sample rate and bit depth are negotiated automatically by the network audio decoder.

```cpp
pinMode(21, OUTPUT);
digitalWrite(21, LOW);
audio.setPinout(13, 11, 12);
audio.setVolume(20);
```

### 4.7 Digital Microphone

- **Device**: LMD3526B261-OFA01/OFA03, powered by 3.3 V, with the channel configured through a resistor on `L/R`.
- **Signals**: MIC CLK and DATA connect to GPIO9 and GPIO10 through the SGM3799 analog switch.
- **Selection**: GPIO45 controls the SGM3799. Verified wireless code pulls GPIO45 low to select wireless SPI, so microphone mode should use the opposite selection state. However, the repository contains no microphone capture example, so **do not hard-code HIGH based solely on this inference**.
- **Recommended verification procedure**: With power off, measure the SGM3799 signal paths or sample both GPIO45 HIGH and LOW states. Then use ESP32-S3 PDM RX with GPIO9 as CLK and GPIO10 as DATA, and confirm the clock with an oscilloscope.
- **Risk**: GPIO9/10 are mutually exclusive with all SPI wireless extensions.

### 4.8 Buzzer

- **Connection**: GPIO8 → 1 kΩ → SS8050 NPN → BEEP_5025; a reverse-protection diode is also included.
- **Method**: Push-pull GPIO switching or LEDC PWM; HIGH turns the NPN on.
- **Software layer**: Arduino GPIO/LEDC.
- **Status**: Confirmed by the schematic, but the repository contains no successfully tested buzzer example. The frequency should be verified according to the physical buzzer type; starting at 2...4 kHz with a 50% duty cycle is recommended.

### 4.9 nRF24L01 Wireless Extension

| Signal | GPIO |
|---|---:|
| SCK / MISO / MOSI | 10 / 9 / 3 |
| CE / CSN | 1 / 2 |
| SS parameter for SPI begin | 46 |
| Analog switch selection | GPIO45 = LOW |

Both the transmitting and receiving ends use address `"00001"`, channel 50 (2.450 GHz), 250 kbps, and `RF24_PA_MAX`. The transmitter calls `stopListening()`, while the receiver calls `openReadingPipe(0, address)` followed by `startListening()`. The software depends on Arduino SPI and the RF24 library.

> GPIO46 is the wireless-interface `W_CS` in the schematic, but the RF24 object actually uses GPIO2 as CSN. The code has the highest priority; GPIO46 is used only as the SS parameter for `SPIClass.begin`. During maintenance, do not incorrectly change the RF24 CSN to GPIO46.

### 4.10 SX1262 LoRa/LoRaWAN Extension

| Signal | GPIO |
|---|---:|
| SPI SCK / MISO / MOSI | 10 / 9 / 3 |
| NSS / DIO1 / NRST / BUSY | 0 / 1 / 2 / 46 |
| Path selection | GPIO45 = LOW |

- **Software layer**: RadioLib `SX1262`, `LoRaWANNode`; supports EU868/US915, with the node constructed for EU868 by default and sub-band set to 1.
- **RF initialization**: `radio.begin()`, current limit of 140 mA, and TCXO voltage of 3.3 V.
- **LoRaWAN**: Supports ABP/OTAA; after OTAA, ADR, DR, duty cycle, 400 ms dwell time, TX power, and RX2 DR are set according to AT parameters.
- **High risk**: The old pin configuration in comments is `NSS=10,DIO1=2,RST=3,BUSY=9`, while the functional code has been changed to `0,1,2,46`. Use the functional constructor as the authoritative configuration. GPIO0 is a bootstrapping pin, so the module NSS must not be held low during reset.

```cpp
SPI.begin(10, 9, 3, 0);
SX1262 radio = new Module(0, 1, 2, 46, SPI);
radio.begin();
radio.setCurrentLimit(140.0);
radio.setTCXO(3.3);
```

### 4.11 UART Zigbee/Other Wireless Modules

- **Pins**: `Serial1` RX GPIO2, TX GPIO1.
- **Parameters**: 115200 baud, 8 data bits, no parity, 1 stop bit (`SERIAL_8N1`).
- **Path selection**: GPIO45 LOW.
- **Software layer**: Arduino HardwareSerial.
- **Note**: The `Zigbee_On_Off_*` examples in lesson-08 are native ESP32-H2 Zigbee examples, not onboard RF drivers for the ESP32-S3 main board. Communication between the main board and an external H2/wireless module should use the UART shown in `zigbee_2.4_2_8.ino`. The ESP32-H2 example values for `BOOT_PIN` and `RGB_BUILTIN` must not be applied to this main board.

### 4.12 USB, Serial Ports, and Download Circuit

- **USB-to-UART port**: USB-C → CH340K → level shifting → ESP32 UART0. DTR/RTS automatically controls EN/GPIO0 through the UMH3NTN to enable automatic downloading.
- **USB OTG port**: USB-C D- → GPIO19, D+ → GPIO20; CC1 and CC2 each have a 5.1 kΩ pull-down, indicating a device-side configuration.
- **Expansion serial ports**: J5 is 5 V UART0 (`TXD0_H`/`RXD0_H`, level-shifted through BSS138); J6 is 3.3 V UART1 (TX GPIO17, RX GPIO18).
- **Safety**: ESP32 GPIOs are not 5 V tolerant. Only the interface labeled 5V-UART0 includes onboard level shifting; 3.3V-UART1 and general-purpose GPIOs must not be connected to 5 V TTL.
- **Drivers**: The PC requires a CH340 driver; the firmware side uses Arduino `Serial`/ESP-IDF UART or TinyUSB/USB drivers.

### 4.13 Buttons, Indicator LEDs, and Auxiliary MCU

- **BOOT K1**: GPIO0, pulled low when pressed; the input normally relies on an internal/external pull-up and also selects download mode.
- **RESET K2**: `CH_EN`; pressing it resets the ESP32.
- **Power LED D13**: 3.3 V connects to the red LED through 5.1 kΩ; this is a hardware-only indicator.
- **Charging indicator D2**: A red/yellow/green multicolor LED is driven by the STC8G1K08 auxiliary MCU according to the TP4059 `CHRG`/`DONE` states; the ESP32 has no direct GPIO connection.
- **Maintenance boundary**: The repository does not contain STC8 firmware, so main-controller porting must not assume that its lighting behavior or charging strategy can be changed.

### 4.14 Power Management

- **Input OR-ing**: USB-UART VBUS, USB-OTG VBUS, and external `+5V_IN` are combined through Schottky diodes at the charger/system input to prevent direct backfeeding between power sources.

- **Charging**: The TP4059 charges the single-cell lithium battery `VBAT`, with a 2 kΩ `PRO` resistor. The actual charging current must be confirmed against the TP4059 datasheet and through measurement; it must not be estimated solely from the resistance value.
- **System Power**: The AO3401 controls the battery/input power path; the RY3420 and a 4.7 µH inductor generate the main power rail, with a 45.3 kΩ/10 kΩ feedback divider.
- **Controlled Outputs**: `3V3_OUT1` and `3V3_OUT2` are generated by high-side P-channel MOSFETs, but their gates are not directly connected to the ESP32 in the schematic. They are board-level power paths and require no initialization by the main controller.
- **Display and Amplifier Switching**: GPIO14 provides optional TFT power control; GPIO21 controls amplifier power; GPIO38 controls only the backlight.
- **Risk**: Both USB ports and the external 5 V supply may be connected simultaneously, but relying on the diodes to parallel different power sources for extended periods is not recommended. During servicing, check the voltage differential, heat generation, and backfeed current.

## 5. External Connector Quick Reference

| Interface | Pins/Signals | Level | Purpose |
|---|---|---|---|
| J5 5V-UART0 | RXD0_H, TXD0_H, +5V_IN, GND | 5 V on UART side | Debugging/external 5 V UART |
| J6 3.3V-UART1 | RX GPIO18, TX GPIO17, 3V3_OUT1, GND | 3.3 V | External UART module |
| J7 3.3V-I2C | SCL GPIO16, SDA GPIO15, 3V3_OUT2, GND | 3.3 V, onboard pull-ups | I2C expansion |
| J8 Wireless Control | RX2/IO2, SCL/IO16, SDA/IO15, 3.3 V, W_CS/IO46, BOOT/IO0, GND | 3.3 V | Control side of wireless adapter board |
| J9 Wireless Bus | TX2/IO1, SCK/IO10, MISO/IO9, MOSI/IO3, 3.3 V, GND, IOT_5V | 3.3 V signals | LoRa/Wi-Fi/Zigbee/NB-IoT/4G adapters |
| J2 Speaker | ROUT+, ROUT- | Differential power output | Speaker; must not be grounded |
| J10 Battery | VBAT, GND | Single-cell lithium battery | Battery input |
| J11 Auxiliary Control/Power | TXD, RXD, VIN, GND | Auxiliary control interface in schematic | STC8 debugging/board testing; not an ESP32 UART |

## 6. Schematic and Code Discrepancy Log

| ID | Item | Schematic/Static Configuration | Active Code | Conclusion and Possible Cause |
|---|---|---|---|---|
| D-01 | Touch INT/RST | GPIO47/GPIO48 | Polling with INT/RST=-1; GPIO1/2 perform the address sequence | Follow the code; possibly a board revision, adapter cable, or legacy code. Physical verification is required |
| D-02 | Touch address | Not specified; LovyanGFX candidate 0x38 (FT5x06) | GT911, 0x5D | `0x38` is a candidate configuration for a different controller and must not be mixed with this configuration |
| D-03 | Touch resolution | LCD 240×320 | touch.h maps to 480×320, then maps to the screen | May support shared code for both 2.4/2.8 models; boundaries must be calibrated |
| D-04 | nRF24 CS | Schematic W_CS=GPIO46 | RF24 CSN=GPIO2, SPI begin SS=GPIO46 | Follow the RF24 constructor; the adapter board may perform secondary remapping |
| D-05 | SX1262 pins | Schematic provides only a generic wireless interface | Commented legacy configuration 10/2/3/9; active configuration 0/1/2/46 | Follow the active `Module(0,1,2,46)` configuration |
| D-06 | LCD controller/touch object | Generic PCB FPC naming | ST7789 + GT911 | The code represents the verified BOM; recheck the IC when sourcing a replacement display |
| D-07 | GPIO18 | Schematic UART1 RX | LVGL setup configures it as OUTPUT but does not use it | Likely a residual statement; RX must be reconfigured before using UART1 |
| D-08 | SD clock | Frequency not specified in schematic | Requests 80 MHz | High-speed configuration depends on the card and PCB routing; reduce the frequency if unstable |

## 7. Pin Conflicts and Combined-Use Constraints

1. **Wireless Expansion and Digital Microphone Are Mutually Exclusive**: GPIO9/10 are selected through the SGM3799, and GPIO45 controls the path selection.
2. **Touch Address Sequence and Wireless Control Are Mutually Exclusive**: The current touch example temporarily drives GPIO1/2; these pins are also used as control pins for nRF24, SX1262, and UART wireless modules.
3. **GPIO0 Boot Risk**: The BOOT button and SX1262 NSS share GPIO0. Any peripheral pulling it low during reset will put the ESP32 into download mode.
4. **GPIO46 Limitation**: ESP32-S3 GPIO46 is a special pin with limited input/output capabilities. The code uses it as a wireless input/SS parameter; do not treat it as a general-purpose high-drive output.
5. **USB GPIO19/20**: After enabling OTG/USB CDC, do not reuse these pins as general-purpose GPIOs or add large capacitors or strong pull-ups.
6. **Shared I2C Bus**: The touch controller and J7 expansion connector share GPIO15/16. New device addresses must not conflict with the touch controller, and the combined pull-up strength must not be excessive.
7. **Dual SPI Hosts**: The LCD uses SPI2_HOST, while the SD/wireless code uses an object named HSPI. Arduino core host enumerations may change; when porting, verify against the actual core version that they map to different controllers.

## 8. Recommended Initialization Sequence

When integrating the display, touch controller, storage, audio, and wireless functions, use the following sequence:

1. Set safe startup states: disable/hold GPIO38 low, place GPIO21 in the amplifier mute state, and use GPIO45 to select the target path.
2. Start the debug UART and output the hardware/firmware versions.
3. Initialize the power-control GPIOs and wait 50...100 ms for power to stabilize.
4. Initialize LCD SPI, DMA, and the backlight; initially keep the brightness low.
5. Initialize I2C and perform the touch reset/address-selection sequence appropriate for the physical hardware revision; read the chip ID or scan for the address.
6. Initialize the SD card, validating first at 20/40 MHz, then increase the frequency based on measured results.
7. Select exactly one path between the microphone and wireless module, then initialize the corresponding I2S/SPI/UART interface.
8. Enable the amplifier or increase the backlight brightness last to prevent power-on pops and overlapping peak current demand.
9. Start LVGL/application tasks; continuously call library functions requiring high-frequency servicing, such as `lv_timer_handler()` and `audio.loop()`, within the loop.

## 9. Porting Checklist

- Pin the versions of Arduino-ESP32, LovyanGFX, LVGL, ESP32-audioI2S, RF24, and RadioLib; APIs and SPI host naming may vary between major versions.
- On a new platform, complete the static GPIO table before porting the buses. Do not directly equate the names `HSPI`/`SPI2_HOST` with physical controllers.
- Preserve the LCD's SPI mode 0, color inversion, and rotation parameters. If colors are incorrect, check `invert`/`rgb_order` before modifying the hardware.
- Add address scanning, chip ID verification, boundary calibration, and graceful failure handling for the touch controller; do not rely solely on a fixed `0x5D` address.
- Add 80→40→20 MHz frequency-reduction retries and card-detection logging for the SD card.
- Assert the GPIO45 mode before initializing wireless drivers, and check the startup states of GPIO0/1/2.
- The audio task must call the decoding loop at high frequency; switch the amplifier to its operating state only after the data is stable.
- Production testing must cover at least: UART, LCD solid colors/orientation, backlight PWM, five-point touch, SD read/write, speaker frequency sweep, MIC recording, buzzer, both USB ports, charging/discharging, and the voltage of every expansion connector.

## 10. Key Risks and Precautions

| Risk Level | Risk | Recommended Action |
|---|---|---|
| High | Touch GPIO1/2 conflicts with schematic GPIO47/48 | Test each PCB revision and define hardware-version macros; do not use one firmware image indiscriminately |
| High | GPIO0 is occupied by LoRa NSS, preventing normal startup | Add a pull-up/isolation and verify the module's power-on state; initialize NSS only after startup |
| High | Wireless and MIC share GPIO9/10 | Use mutually exclusive resource management; stop the peripheral and set the pins to high impedance before switching |
| High | 3.3 V interface accidentally connected to 5 V | Only J5 5V-UART0 may be connected to 5 V TTL; treat all others as 3.3 V |
| High | Differential speaker terminal accidentally grounded | Connect ROUT+/ROUT- only to a floating speaker load |
| Medium | Insufficient signal margin at SD 80 MHz | Provide frequency-reduction retries and test production cards across temperature and manufacturing lots |
| Medium | LVGL dual full-screen buffers depend on PSRAM | Check allocation return values; use tiled/partial rendering when PSRAM is unavailable |
| Medium | Confusion over LCD/SD/wireless SPI host naming | Print or review host mappings on the target Arduino core to avoid conflicts on the same host |
| Medium | Simultaneously enabling high amplifier volume and the backlight causes a power drop | Stagger startup, monitor 3.3 V/VIN, and ramp up the volume |
| Medium | Software drives optional components marked NC in the schematic | Verify the BOM/physical hardware first, especially the GPIO14 TFT power circuit and optional MOSFETs |
| Low | Inconsistent UART baud rates in examples | Standardize maintenance firmware on 115200 and document it in the README |

## 11. Evidence Index

| Content | Project Files |
|---|---|
| V1.2 electrical connections, BOM, net names | `1.2/ESP32 Display 2.8 inch V1.2(1).sch`, and the PDF with the same name |
| LCD, touch, LVGL, backlight | `lesson-03/2.4_2.8_LVGL/`, `lesson-03/pwn version/pwm/` |
| I2S amplifier and online audio | `lesson-02/OnlineAudio_small/OnlineAudio_small.ino` |
| microSD | `lesson-04/SD_CrowPanel_ESP32_Advance_HMI_2.4_2.8/` |
| nRF24 transmit/receive | `lesson-06/READ/`, `lesson-06/WRITE/` |
| SX1262/LoRaWAN | `lesson-07/code/sendATcommands_2.4_2_8/` |
| UART wireless expansion | `lesson-08/zigbee_2.4_2_8/zigbee_2.4_2_8.ino` |
| Basic UART | `lesson-01/Serial_port_printing/Serial_port_printing.ino` |

## 12. Maintenance Log Template

Add the following record after every subsequent hardware or driver change to prevent the schematic, BOM, and code from becoming inconsistent again:

| Date | PCB/BOM Version | Firmware Version/Commit | Peripheral | Change | Test Result | Owner |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | Vx.x | commit/tag | Device name | Pin/parameter/polarity change | Pass/Fail + conditions | Name |