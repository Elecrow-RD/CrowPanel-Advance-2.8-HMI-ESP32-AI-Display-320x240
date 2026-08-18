#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <driver/i2c.h>

/**
 * @brief Configure LovyanGFX for the CrowPanel SPI display and touch panel.
 *
 * The constructor binds the SPI bus to the ST7789 controller, describes the
 * visible panel geometry, and attaches the FT5x06 touch controller. Arduino
 * creates the global display object before setup(), so the hardware profile is
 * ready when gfx.init() is called.
 */
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Touch_FT5x06 _touch_instance;

  public:
    LGFX(void)
    {
      /*---------------------------------------------------------------
       * Configure the SPI bus used by the LCD controller
       * DMA and bus locking support efficient transfers while allowing
       * the display and SD card to use their assigned SPI resources.
       *--------------------------------------------------------------*/
      {
        auto cfg = _bus_instance.config();

        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 80000000;
        cfg.freq_read = 16000000;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = 42;
        cfg.pin_mosi = 39;
        cfg.pin_miso = -1;
        cfg.pin_dc = 41;

        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
      }

      /*---------------------------------------------------------------
       * Describe the ST7789 panel geometry and signal behavior
       * These values align the controller memory with the physical
       * 240-by-320 panel and match its color and rotation wiring.
       *--------------------------------------------------------------*/
      {
        auto cfg = _panel_instance.config();

        cfg.pin_cs = 40;
        cfg.pin_rst = -1;
        cfg.pin_busy = -1;
        cfg.memory_width = 240;
        cfg.memory_height = 320;
        cfg.panel_width = 240;
        cfg.panel_height = 320;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 3;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = false;
        cfg.invert = true;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.bus_shared = true;

        _panel_instance.config(cfg);
      }

      /*---------------------------------------------------------------
       * Configure the capacitive touch controller
       * The coordinate range and rotation map raw FT5x06 points to the
       * panel, while I2C provides the controller communication channel.
       *--------------------------------------------------------------*/
      {
        auto cfg = _touch_instance.config();

        cfg.x_min = 0;
        cfg.x_max = 239;
        cfg.y_min = 0;
        cfg.y_max = 319;
        cfg.pin_int = 47;
        cfg.bus_shared = false;
        cfg.offset_rotation = 6;
        cfg.i2c_port = 0;
        cfg.i2c_addr = 0x38;
        cfg.pin_sda = 15;
        cfg.pin_scl = 16;
        cfg.freq = 400000;

        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
      }

      setPanel(&_panel_instance);
    }
};
