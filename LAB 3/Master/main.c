/*
 * Master.c
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
#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "spi.h"

/****************************************/
// Function prototypes

void leds_init(void);
void mostrar_leds(uint8_t numero);
void procesar_uart(void);
void mostrar_pots_uart(uint8_t pot1, uint8_t pot2);

/****************************************/
// Main Function

uint8_t numero = 0;
uint16_t acumulado = 0;
uint8_t escribiendo = 0;
uint8_t pot1 = 0;
uint8_t pot2 = 0;

int main(void)
{
	uart_init();
	spi_master_init();
	leds_init();
	uart_print("Ingrese un numero de 0 a 255:\r\n");

	while (1)
	{
		while (uart_hay_dato())
		{
			procesar_uart();
		}

		spi_master_exchange(numero, &pot1, &pot2);
		mostrar_pots_uart(pot1, pot2);
		_delay_ms(200);
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
	if (numeroLed & (1 << 0)) PORTD |= (1 << PD2); else PORTD &= ~(1 << PD2);
	if (numeroLed & (1 << 1)) PORTD |= (1 << PD3); else PORTD &= ~(1 << PD3);
	if (numeroLed & (1 << 2)) PORTD |= (1 << PD4); else PORTD &= ~(1 << PD4);
	if (numeroLed & (1 << 3)) PORTD |= (1 << PD5); else PORTD &= ~(1 << PD5);
	if (numeroLed & (1 << 4)) PORTD |= (1 << PD6); else PORTD &= ~(1 << PD6);
	if (numeroLed & (1 << 5)) PORTD |= (1 << PD7); else PORTD &= ~(1 << PD7);
	if (numeroLed & (1 << 6)) PORTB |= (1 << PB0); else PORTB &= ~(1 << PB0);
	if (numeroLed & (1 << 7)) PORTB |= (1 << PB1); else PORTB &= ~(1 << PB1);
}

void procesar_uart(void)
{
	char dato;
	char texto[40];

	dato = uart_rx();

	if (dato >= '0' && dato <= '9')
	{
		escribiendo = 1;
		acumulado = (acumulado * 10) + (dato - '0');
		uart_tx(dato);
	}
	else if ((dato == '\r' || dato == '\n') && escribiendo == 1)
	{
		uart_print("\r\n");

		if (acumulado <= 255)
		{
			numero = (uint8_t)acumulado;
			mostrar_leds(numero);
			snprintf(texto, sizeof(texto), "Valor LEDs: %u\r\n", numero);
			uart_print(texto);
		}
		else
		{
			uart_print("Numero invalido. Use 0 a 255\r\n");
		}

		acumulado = 0;
		escribiendo = 0;
		uart_print("Ingrese un numero de 0 a 255:\r\n");
	}
}

void mostrar_pots_uart(uint8_t valor1, uint8_t valor2)
{
	static uint8_t pot1_anterior = 255;
	static uint8_t pot2_anterior = 255;
	static uint8_t contador_muestras = 0;
	char texto[40];

	contador_muestras++;

	if (contador_muestras < 5)
	{
		return;
	}

	contador_muestras = 0;

	if (valor1 != pot1_anterior || valor2 != pot2_anterior)
	{
		snprintf(texto, sizeof(texto), "Pot1:%u Pot2:%u\r\n", valor1, valor2);
		uart_print(texto);
		pot1_anterior = valor1;
		pot2_anterior = valor2;
	}
}

/****************************************/
// Interrupt routines
