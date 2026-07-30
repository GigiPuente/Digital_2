/*
 * spi.c
 *
 * Created: 
 * Author: 
 * Description: 
 *
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "spi.h"

#define SS1   PB2
#define MOSI1 PB3
#define MISO1 PB4
#define SCK1  PB5

/****************************************/
// Function prototypes

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines

void spi_master_init(void)
{
	DDRB |= (1 << SS1) | (1 << MOSI1) | (1 << SCK1);
	DDRB &= ~(1 << MISO1);
	PORTB |= (1 << SS1);
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
}

uint8_t spi_master_transfer(uint8_t data)
{
	SPDR = data;

	while (!(SPSR & (1 << SPIF)))
	{
	}

	return SPDR;
}

void spi_master_exchange(uint8_t led_value, uint8_t *pot1, uint8_t *pot2)
{
	PORTB |= (1 << SS1);
	_delay_us(50);
	PORTB &= ~(1 << SS1);
	_delay_us(50);

	*pot1 = spi_master_transfer(led_value);
	_delay_us(50);
	*pot2 = spi_master_transfer(0x00);
	_delay_us(50);

	PORTB |= (1 << SS1);
	_delay_us(50);
}

/****************************************/
// Interrupt routines
