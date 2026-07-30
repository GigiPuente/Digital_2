#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

void spi_master_init(void);
uint8_t spi_master_transfer(uint8_t data);
void spi_master_exchange(uint8_t led_value, uint8_t *pot1, uint8_t *pot2);

#endif /* SPI_H_ */
