#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

void spi_slave_init(void);
void spi_slave_update_pots(uint8_t pot1, uint8_t pot2);
uint8_t spi_slave_get_led_data(void);

#endif /* SPI_H_ */
