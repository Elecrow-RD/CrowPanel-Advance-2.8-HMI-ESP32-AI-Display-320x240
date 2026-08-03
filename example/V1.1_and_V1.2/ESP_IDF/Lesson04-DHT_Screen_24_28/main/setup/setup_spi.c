#include "setup_spi.h"
#include "setup_gpio.h"

void setup_spi_init(void)
{
    spi_bus_config_t spi_cfg = {};
    spi_cfg.sclk_io_num = LCD_SCLK_NUM;
    spi_cfg.mosi_io_num = LCD_MOSI_NUM;
    spi_cfg.miso_io_num = LCD_MISO_NUM;
    spi_cfg.quadwp_io_num = GPIO_NUM_NC;
    spi_cfg.quadhd_io_num = GPIO_NUM_NC;
    spi_cfg.max_transfer_sz = LCD_H_RES_DATA * LCD_BUFFER_H_DATA * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI2_HOST, &spi_cfg, SPI_DMA_CH_AUTO));
}
