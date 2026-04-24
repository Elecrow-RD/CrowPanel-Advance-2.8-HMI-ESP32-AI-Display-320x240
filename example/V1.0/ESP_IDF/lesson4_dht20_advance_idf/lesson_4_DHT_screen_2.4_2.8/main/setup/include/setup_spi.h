#ifndef SETUP_SPI_H_
#define SETUP_SPI_H_

#include "driver/spi_master.h"

#define LCD_SPI2_HOST SPI2_HOST

#define LCD_H_RES_DATA (320)
#define LCD_BUFFER_H_DATA (50)

void setup_spi_init(void);

#endif
