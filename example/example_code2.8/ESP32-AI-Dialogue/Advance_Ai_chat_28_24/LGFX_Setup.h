
#ifndef LGFX_SETUP_H
#define LGFX_SETUP_H

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{

// Instance of the display panel driver
    lgfx::Panel_ST7789 _panel_instance;
    // Instance for the bus type connected to the screen (SPI in this case)
    lgfx::Bus_SPI _bus_instance;


public:

     // Constructor to configure various settings.
    // If you change the class name, make sure to specify the same name for the constructor.
  LGFX(void)
  {
	  // Configure the bus control settings.
    {  

      auto cfg = _bus_instance.config();   	 // Get the bus configuration structure.

// SPI settings
      cfg.spi_host = SPI2_HOST;      // Select the SPI to use. For ESP32-S2, C3: SPI2_HOST or SPI3_HOST; for ESP32: VSPI_HOST or HSPI_HOST.
 // * With the upgrade of the ESP-IDF version, the descriptions of VSPI_HOST and HSPI_HOST are deprecated.
 //   If an error occurs, please use SPI2_HOST and SPI3_HOST instead.
      cfg.spi_mode = 0;             // Set the SPI communication mode (0 ~ 3).
      cfg.freq_write = 80000000;                // SPI clock during transmission (maximum 80MHz, rounded to an integer multiple of 80MHz).
      cfg.freq_read  = 16000000;    // SPI clock during reception.
      cfg.spi_3wire  = false;        // Set to true if receiving data using the MOSI pin.
      cfg.use_lock   = true;        /// Set to true if using a transaction lock.
      cfg.dma_channel = 2;             // Set the DMA channel to use (0 = no DMA, 1 = channel 1, 2 = channel 2, SPI_DMA_CH_AUTO = automatic setting).

        // * With the upgrade of the ESP-IDF version, it is now recommended to use SPI_DMA_CH_AUTO (automatic setting)
            //   as the DMA channel. Channels 1 and 2 are deprecated.
      cfg.pin_sclk = 42;             // Set the pin number for the SPI SCLK.
      cfg.pin_mosi = 39;             // Set the pin number for the SPI MOSI.
      cfg.pin_miso = -1;             // Set the pin number for the SPI MISO (-1 = disable).
      cfg.pin_dc   = 41;            // Set the pin number for the SPI D/C (-1 = disable).

      // When sharing the SPI bus with an SD card, the MISO pin must be set without omission.


      _bus_instance.config(cfg);    // Apply the configuration to the bus.
      _panel_instance.setBus(&_bus_instance);       // Set the bus for the panel.
    }

        // Configure the display panel control settings.
        {
            // Get the panel configuration structure.
            auto cfg = _panel_instance.config();

            // Pin number connected to CS (-1 = disable).
            cfg.pin_cs = 40;
            // Pin number connected to RST (-1 = disable).
            cfg.pin_rst = -1;
            // Pin number connected to BUSY (-1 = disable).
            cfg.pin_busy = -1;

            // * The following settings have general default values for each panel.
            //   If you're unsure about a certain item, you can comment it out and try.

            // Actual displayable width.
            cfg.panel_width = 240;
            // Actual displayable height.
            cfg.panel_height = 320;
            // Offset in the X direction of the screen.
            cfg.offset_x = 0;
            // Offset in the Y direction of the screen.
            cfg.offset_y = 0;
            // Offset in the rotation direction (0 ~ 7, 4 ~ 7 are inverted).
            cfg.offset_rotation = 0;
            // Number of dummy read bits before reading a pixel.
            cfg.dummy_read_pixel = 8;
            // Number of dummy read bits before reading non - pixel data.
            cfg.dummy_read_bits = 1;
            // Set to true if data can be read from the panel.
            cfg.readable = false;
            // Set to true if the panel's brightness is inverted.
            cfg.invert = true;
            // true for RGB, false for BGR.
            cfg.rgb_order = false;
            // Set to true if the panel transfers data length in 16 - bit units in 16 - bit parallel or SPI.
            cfg.dlen_16bit = false;
            // Set to true if the bus is shared with an SD card (bus control is performed by functions like drawJpgFile).
            cfg.bus_shared = true;

            // Apply the configuration to the panel.
            _panel_instance.config(cfg);
        }

    setPanel(&_panel_instance);         // Set the panel to be used.
  }
};




extern LGFX TFT;  // Declare the TFT object for use in the main file.

#endif
