#include <SPI.h>
#include <Wire.h>
#include <nRF24L01.h>
#include <RF24.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789     _panel_instance;
    lgfx::Bus_SPI       _bus_instance;   // Instance of the SPI bus
    // lgfx::Touch_FT5x06  _touch_instance;

  public:
    LGFX(void) {
      {
        auto cfg = _bus_instance.config();

        // SPI bus configuration
        cfg.spi_host = SPI2_HOST;  // Select the SPI to use: ESP32-S2, C3: SPI2_HOST or SPI3_HOST / ESP32: VSPI_HOST or HSPI_HOST
        // * With ESP-IDF version upgrades, the descriptions VSPI_HOST and HSPI_HOST are deprecated. If an error occurs, use SPI2_HOST or SPI3_HOST instead. 
        cfg.spi_mode = 0;                   // Set SPI communication mode (0 ~ 3)
        cfg.freq_write = 80000000;          // SPI clock during transmission (Max 80MHz, rounded down to an integer division of 80MHz)
        cfg.freq_read = 16000000;           // SPI clock during reception
        cfg.spi_3wire = false;               // Set to true if reception is performed on the MOSI pin
        cfg.use_lock = true;                // Set to true when using transaction locks
        cfg.dma_channel = SPI_DMA_CH_AUTO;  // Set the DMA channel to use (0=do not use DMA / 1=1ch / 2=2ch / SPI_DMA_CH_AUTO=automatic setting)
        // * With ESP-IDF version upgrades, SPI_DMA_CH_AUTO (automatic setting) is recommended for the DMA channel. Specifying 1ch or 2ch is deprecated.
        cfg.pin_sclk = 42;  // Set the SPI SCLK pin number
        cfg.pin_mosi = 39;  // Set the SPI MOSI pin number
        cfg.pin_miso = -1;  // Set the SPI MISO pin number (-1 = disable)
        cfg.pin_dc = 41;     // Set the SPI D/C pin number (-1 = disable)

        _bus_instance.config(cfg);               // Apply configuration values to the bus.
        _panel_instance.setBus(&_bus_instance);  // Set the bus to the panel.
      }

      { // Configure the display panel control settings.
        auto cfg = _panel_instance.config();  // Get the configuration structure for display panel settings.

        cfg.pin_cs = 40;    // Pin number connected to CS (-1 = disable)
        //        cfg.pin_rst = 9;   // Pin number connected to RST (-1 = disable)
        cfg.pin_rst = -1;   // Pin number connected to RST (-1 = disable)
        cfg.pin_busy = -1;  // Pin number connected to BUSY (-1 = disable)

        // * The following default configuration values are generally pre-set for each panel type, so if there are unknown items, you can comment them out and test them.

        cfg.memory_width = 240;    // Maximum width supported by the driver IC
        cfg.memory_height = 320;   // Maximum height supported by the driver IC
        cfg.panel_width = 240;     // Actually displayable width
        cfg.panel_height = 320;    // Actually displayable height
        cfg.offset_x = 0;          // Offset amount in the X direction of the panel
        cfg.offset_y = 0;          // Offset amount in the Y direction of the panel
        cfg.offset_rotation = 3;   // Offset value for rotation direction 0~7 (4~7 are inverted)
        cfg.dummy_read_pixel = 8;  // Number of dummy bits to read before reading pixels
        cfg.dummy_read_bits = 1;   // Number of dummy bits to read before reading data other than pixels
        cfg.readable = false;      // Set to true if data can be read from the panel
        cfg.invert = true;         // Set to true if the panel's highlights and shadows are inverted
        cfg.rgb_order = false;      // Set to true if the panel's red and blue channels are swapped
        cfg.dlen_16bit = false;    // Set to true for panels that transmit data lengths in 16-bit units
        cfg.bus_shared = true;    // Set to true if the bus is shared with an SD card (enables bus control when executing drawJpgFile etc.)

        _panel_instance.config(cfg);
      }

      // { // Configure touchscreen control settings (Delete if not needed)
      //   auto cfg = _touch_instance.config();

      //   cfg.x_min = 0;            // Minimum X value obtained from the touchscreen (raw value)
      //   cfg.x_max = 239;          // Maximum X value obtained from the touchscreen (raw value)
      //   cfg.y_min = 0;            // Minimum Y value obtained from the touchscreen (raw value)
      //   cfg.y_max = 319;          // Maximum Y value obtained from the touchscreen (raw value)
      //   cfg.pin_int = 47;         // Pin number connected to INT
      //   cfg.bus_shared = false;   // Set to true if you are using the same bus as the screen
      //   cfg.offset_rotation = 6;  // Adjustment when display and touch orientation don't match. Set a value from 0 to 7

      //   // In case of I2C connection
      //   cfg.i2c_port = 0;     // Select the I2C port to use (0 or 1)
      //   cfg.i2c_addr = 0x38;  // I2C device address number
      //   cfg.pin_sda = 15;     // Pin number connected to SDA
      //   cfg.pin_scl = 16;     // Pin number connected to SCL
      //   cfg.freq = 400000;    // Set the I2C clock frequency

      //   _touch_instance.config(cfg);
      //   _panel_instance.setTouch(&_touch_instance);  // Set the touchscreen to the panel.
      // }
      setPanel(&_panel_instance);
    }
};

LGFX gfx;

#define CE_PIN 1
#define CSN_PIN 2

// Controls the nRF24L01 module through the CE and CSN pins.
RF24 radio(CE_PIN, CSN_PIN);

// Uses a separate SPI object so the radio can use its own pin mapping.
SPIClass* hspi = nullptr;

/*---------------------------------------------------------------
 * nRF24L01 SPI pins
 * These pins connect the wireless module to the ESP32-S3 HSPI bus.
 * The same mapping must be used by both the transmitter and receiver
 * sketches in this lesson pair.
 *--------------------------------------------------------------*/
#define HSPI_MISO  9
#define HSPI_MOSI  3
#define HSPI_SCLK  10
#define HSPI_SS    46

/**
 * @brief Show the current transmit status on the LCD.
 *
 * The transmitter updates the display once per payload so students can see
 * that the sending loop is running even before checking the receiver board.
 *
 * @param lcd_w Display width reserved for future layout adjustments.
 * @param lcd_h Display height reserved for future layout adjustments.
 * @param x Text start position on the X axis.
 * @param y Text start position on the Y axis.
 * @param text Null-terminated message to draw.
 * @return None.
 */
void show_test(int lcd_w, int lcd_h, int x, int y, const char * text)
{
  gfx.fillScreen(TFT_BLACK);
  gfx.setTextSize(3);
  gfx.setTextColor(TFT_RED);
  gfx.setCursor(x, y);
  gfx.print(text);
}

// Radio pipe address shared with the receiver sketch.
const byte address[6] = "00001";

/**
 * @brief Initialize the LCD and configure the nRF24L01 transmitter.
 *
 * Arduino calls this function once after reset. It enables the wireless
 * module path, starts the radio on the configured SPI pins, opens the writing
 * pipe, and puts the radio into transmit mode.
 *
 * @param None.
 * @return None.
 */
void setup() {
  Serial.begin(115200);

  Wire.begin(15, 16);
  delay(50);

  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);

  /*---------------------------------------------------------------
   * Enable the wireless module path
   * GPIO45 selects the wireless module instead of the alternate audio
   * path on this board.
   *--------------------------------------------------------------*/
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);

  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

 while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  /*---------------------------------------------------------------
   * Start the radio as a transmitter
   * Channel, data rate, and address must match the receiver or the
   * payloads will be sent without a matching listener.
   *--------------------------------------------------------------*/
  hspi = new SPIClass(HSPI);
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);

  if (!radio.begin(hspi)) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  else
  {
    Serial.println(F("radio hardware is OK!!"));
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(50);
  radio.stopListening();
}

// Counts transmitted payloads so the LCD text visibly changes over time.
int i = 0;

/**
 * @brief Send a payload once per second.
 *
 * Arduino calls this function repeatedly after setup() finishes. Each pass
 * updates the LCD, sends a fixed text payload through the radio, and waits
 * long enough for the receiver display to be observed.
 *
 * @param None.
 * @return None.
 */
void loop() {
  Serial.println(F("SENDING..."));
  String str = "SENDING...";
  str += String(i);
  show_test(320, 240, 50, 100, str.c_str());
  i++;
  const char text[] = "Hello World I3:";
  radio.write(&text, sizeof(text));
  delay(1000);
}
