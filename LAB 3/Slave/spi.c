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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"

#define SS1   PB2
#define MOSI1 PB3
#define MISO1 PB4
#define SCK1  PB5

volatile uint8_t datoLed = 0;
volatile uint8_t potBuffer[2] = {0, 0};
volatile uint8_t contador = 0;

/****************************************/
// Function prototypes

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines

void spi_slave_init(void)
{
	DDRB &= ~((1 << SS1) | (1 << MOSI1) | (1 << SCK1));
	DDRB |= (1 << MISO1);
	SPCR = (1 << SPE) | (1 << SPIE);
	PCICR |= (1 << PCIE0);
	PCMSK0 |= (1 << PCINT2);
	contador = 0;
	SPDR = potBuffer[0];
	sei();
}

void spi_slave_update_pots(uint8_t pot1, uint8_t pot2)
{
	uint8_t copiaSREG = SREG;

	cli();
	potBuffer[0] = pot1;
	potBuffer[1] = pot2;
	SREG = copiaSREG;
}

uint8_t spi_slave_get_led_data(void)
{
	return datoLed;
}

/****************************************/
// Interrupt routines

ISR(SPI_STC_vect)
{
	if (contador == 0)
	{
		datoLed = SPDR;
		contador = 1;
		SPDR = potBuffer[1];
	}
	else
	{
		contador = 0;
		SPDR = potBuffer[0];
	}
}

ISR(PCINT0_vect)
{
	if (!(PINB & (1 << SS1)))
	{
		contador = 0;
		SPDR = potBuffer[0];
	}
}
