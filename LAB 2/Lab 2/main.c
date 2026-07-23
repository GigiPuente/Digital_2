/*
 * main.c
 *
 * Created: 16/07/2026
 * Author: Jorge Puente
 * Description:
 *
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"

/****************************************/
// Function prototypes

static void adc_init(void);
static uint16_t leer_ADC(uint8_t canal);
static void pasar_a_voltaje(uint16_t num, char *texto);

/****************************************/
// Main Function

int main(void)
{
	//Texto para la fila de arriba
	char texto1[17];
	//Texto para la fila de abajo
	char texto2[17];
	//Texto de S1
	char v1[5];
	//Texto de S2
	char v2[5];
	//Texto de S3
	char v3[5];
	//Valor de S1
	uint16_t s1;
	//Valor de S2
	uint16_t s2;
	//Valor de S3
	uint16_t s3;

	//Iniciar LCD
	lcd_init();
	//Iniciar ADC
	adc_init();

	while (1)
	{
		//Leer S1 en A4
		s1 = leer_ADC(4);
		//Leer S2 en A5
		s2 = leer_ADC(5);
		//Leer S3 en A6
		s3 = leer_ADC(6);

		//Pasar S1 a voltaje
		pasar_a_voltaje(s1, v1);
		//Pasar S2 a voltaje
		pasar_a_voltaje(s2, v2);
		//Pasar S3 a voltaje
		pasar_a_voltaje(s3, v3);

		//Texto de la fila de arriba
		snprintf(texto1, sizeof(texto1), "S1    S2    S3  ");
		//Texto de la fila de abajo
		snprintf(texto2, sizeof(texto2), "%s  %s  %s", v1, v2, v3);

		//Mostrar titulos
		lcd_posicion(0, 0);
		lcd_print(texto1);

		//Mostrar voltajes alineados
		lcd_posicion(1, 0);
		lcd_print(texto2);

		//Esperar un poco
		_delay_ms(200);
	}
}

/****************************************/
// NON-Interrupt subroutines

static void adc_init(void)
{
	//AVcc referencia
	ADMUX = (1 << REFS0);
	//ADC con prescaler de 128
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static uint16_t leer_ADC(uint8_t canal)
{
	//Canal ADC
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F) | (1 << REFS0);
	//Conversion
	ADCSRA |= (1 << ADSC);

	//Esperar a que se haga la conversion
	while (ADCSRA & (1 << ADSC))
	{
	}

	//Devolver el valor que se leyo del ADC
	return ADC;
}

static void pasar_a_voltaje(uint16_t num, char *texto)
{
	//Voltaje en centesimas
	uint16_t vol;
	//Parte entera
	uint8_t ent;
	//Parte decimal
	uint8_t dec;

	//Pasar ADC a voltaje
	vol = (uint16_t)(((uint32_t)num * 500U) / 1023U);
	ent = vol / 100;
	dec = vol % 100;

	//Guardar como 0.00
	snprintf(texto, 5, "%u.%02u", ent, dec);
}

/****************************************/
// Interrupt routines
