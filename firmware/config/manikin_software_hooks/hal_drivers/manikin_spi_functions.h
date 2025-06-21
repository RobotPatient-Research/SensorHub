#ifndef MANIKIN_SPI_FUNCTIONS_H
#define MANIKIN_SPI_FUNCTIONS_H
#include "manikin_software_conf.h"
#include <stddef.h>

int spi_hal_init(manikin_spi_inst_t spi_inst, uint32_t baud);

int spi_hal_set_cs(manikin_spi_cs_t spi_cs, uint8_t state);

size_t spi_hal_write_bytes(manikin_spi_inst_t spi_inst, uint8_t *data, size_t len);

size_t spi_hal_read_bytes(manikin_spi_inst_t spi_inst, uint8_t *data, size_t len);

int spi_hal_deinit(manikin_spi_inst_t spi_inst);

#endif /* MANIKIN_SPI_FUNCTIONS_H */