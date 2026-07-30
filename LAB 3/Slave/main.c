/*
 * Slave.c
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
#include <stdint.h>
#include "adc.h"
#include "spi.h"

/****************************************/
// Function prototypes

void leds_init(void);
void mostrar_leds(uint8_t numero);

/****************************************/
// Main Function

int main(void)
{
	uint8_t pot1 = 0;
	uint8_t pot2 = 0;
	uint8_t numero = 0;

	adc_init();
	spi_slave_init();
	leds_init();

	while (1)
	{
		pot1 = adc_read(4);
		pot2 = adc_read(5);
		spi_slave_update_pots(pot1, pot2);
		numero = spi_slave_get_led_data();
		mostrar_leds(numero);
		_delay_ms(10);
	}
}

/****************************************/
// NON-Interrupt subroutines

void leds_init(void)
{
	DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);
	DDRB |= (1 << PB0) | (1 << PB1);
}

void mostrar_leds(uint8_t numeroLed)
{
	if (numeroLed & (1 << 0)) PORTB |= (1 << PB1); else PORTB &= ~(1 << PB1);
	if (numeroLed & (1 << 1)) PORTD |= (1 << PD2); else PORTD &= ~(1 << PD2);
	if (numeroLed & (1 << 2)) PORTD |= (1 << PD3); else PORTD &= ~(1 << PD3);
	if (numeroLed & (1 << 3)) PORTD |= (1 << PD4); else PORTD &= ~(1 << PD4);
	if (numeroLed & (1 << 4)) PORTD |= (1 << PD5); else PORTD &= ~(1 << PD5);
	if (numeroLed & (1 << 5)) PORTD |= (1 << PD6); else PORTD &= ~(1 << PD6);
	if (numeroLed & (1 << 6)) PORTD |= (1 << PD7); else PORTD &= ~(1 << PD7);
	if (numeroLed & (1 << 7)) PORTB |= (1 << PB0); else PORTB &= ~(1 << PB0);
}

/****************************************/
// Interrupt routines
