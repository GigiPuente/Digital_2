/*
 * adc.c
 *
 * Created:
 * Author:
 * Description: 
 */
/****************************************/
// Encabezado (Libraries)

#include <avr/io.h>
#include "adc.h"

/****************************************/
// Function prototypes

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines

void adc_init(void)
{
	// AVcc como referencia y ajuste a la izquierda para usar ADCH
	ADMUX = (1 << REFS0) | (1 << ADLAR);

	// Encender ADC con prescaler 128
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint8_t adc_read(uint8_t channel)
{
	// Elegir canal
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F) | (1 << REFS0) | (1 << ADLAR);

	// Primera conversion
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC))
	{
	}

	// Segunda conversion para asegurar lectura estable
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC))
	{
	}

	return ADCH;
}

/****************************************/
// Interrupt routines
