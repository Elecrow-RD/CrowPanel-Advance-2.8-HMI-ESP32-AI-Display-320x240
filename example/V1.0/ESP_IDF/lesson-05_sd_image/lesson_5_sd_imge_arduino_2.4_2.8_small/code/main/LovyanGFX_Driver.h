#define LGFX_USE_V1
#include <LovyanGFX.hpp>
//#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
//#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>

class LGFX : public lgfx::LGFX_Device
{
   lgfx::Panel_ST7789     _panel_instance;
//    lgfx::Panel_ILI9488     _panel_instance;
    // lgfx::Panel_ILI9341     _panel_instance;
    lgfx::Bus_SPI       _bus_instance;   // Instance of SPI bus
    lgfx::Touch_FT5x06  _touch_instance;

  public:
    LGFX(void) {
      {
        auto cfg = _bus_instance.config();

        // Configure SPI bus
        cfg.spi_host = SPI2_HOST;  // Select SPI to use. ESP32-S2, C3: SPI2_HOST or SPI3_HOST / ESP32: VSPI_HOST or HSPI_HOST
        // * As ESP-IDF versions update, VSPI_HOST and HSPI_HOST become deprecated. Use SPI2_HOST or SPI3_HOST instead if errors occur.
        cfg.spi_mode = 0;                   // Set SPI communication mode (0 ~ 3)
        cfg.freq_write = 80000000;          // SPI clock for writing (max 80MHz, rounded to an integer division of 80MHz)
        cfg.freq_read = 16000000;           // SPI clock for reading
        cfg.spi_3wire = false;              // Set to true when receiving via MOSI pin
        cfg.use_lock = true;                // Set to true to use transaction lock
        cfg.dma_channel = SPI_DMA_CH_AUTO;  // Set the DMA channel to use (0 = no DMA / 1 = ch1 / 2 = ch2 / SPI_DMA_CH_AUTO = auto)
        // * As ESP-IDF updates, SPI_DMA_CH_AUTO is recommended for automatic DMA channel setting. Manual 1ch/2ch is deprecated.
        cfg.pin_sclk = 42;  // Set pin number for SPI SCLK
        cfg.pin_mosi = 39;  // Set pin number for SPI MOSI
        cfg.pin_miso = -1;  // Set pin number for SPI MISO (-1 = disable)
        cfg.pin_dc = 41;    // Set pin number for SPI D/C (-1 = disable)

        _bus_instance.config(cfg);               // Apply the settings to the bus
        _panel_instance.setBus(&_bus_instance);  // Set the bus to the panel
      }

      { // Configure display panel control
        auto cfg = _panel_instance.config();  // Get structure for panel settings

        cfg.pin_cs = 40;    // Pin number connected to CS (-1 = disable)
        //        cfg.pin_rst = 9;   // Pin number connected to RST (-1 = disable)
        cfg.pin_rst = -1;   // Pin number connected to RST (-1 = disable)
        cfg.pin_busy = -1;  // Pin number connected to BUSY (-1 = disable)

        // * The following values are typical defaults for each panel. If uncertain, comment out and try.

        cfg.memory_width = 240;    // Max width supported by driver IC
        cfg.memory_height = 320;   // Max height supported by driver IC
        cfg.panel_width = 240;     // Actual visible width
        cfg.panel_height = 320;    // Actual visible height
        cfg.offset_x = 0;          // Offset in X direction
        cfg.offset_y = 0;          // Offset in Y direction
        cfg.offset_rotation = 3;   // Offset for rotation direction (0~7, 4~7 = flipped)
        cfg.dummy_read_pixel = 8;  // Dummy bits read before pixel read
        cfg.dummy_read_bits = 1;   // Dummy bits read before reading data other than pixels
        cfg.readable = false;      // Set true if data can be read from the panel
        cfg.invert = true;         // Set true if brightness is inverted
        cfg.rgb_order = false;     // Set true if red and blue are swapped
        cfg.dlen_16bit = false;    // Set true for 16-bit length data transmission panels
        cfg.bus_shared = true;     // Set true if the bus is shared with SD card (e.g., for drawJpgFile)

        _panel_instance.config(cfg);
      }

      { // Configure touchscreen control (delete if not needed)
        auto cfg = _touch_instance.config();

        cfg.x_min = 0;            // Minimum X value from touchscreen (raw value)
        cfg.x_max = 239;          // Maximum X value from touchscreen (raw value)
        cfg.y_min = 0;            // Minimum Y value from touchscreen (raw value)
        cfg.y_max = 319;          // Maximum Y value from touchscreen (raw value)
        cfg.pin_int = 47;         // Pin number connected to INT
        cfg.bus_shared = false;   // Set true if sharing the bus with the screen
        cfg.offset_rotation = 6;  // Rotation adjustment if display and touch direction mismatch (0~7)

        // For I2C connection
        cfg.i2c_port = 0;     // Select I2C port to use (0 or 1)
        cfg.i2c_addr = 0x38;  // I2C device address
        cfg.pin_sda = 15;     // Pin number connected to SDA
        cfg.pin_scl = 16;     // Pin number connected to SCL
        cfg.freq = 400000;    // Set I2C clock

        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);  // Set touchscreen to the panel
      }

      setPanel(&_panel_instance);
    }
};
