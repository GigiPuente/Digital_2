/*
 * uart.c
 *
 * Created: 22/07/2026
 * Author: Jorge Puente
 * Description:
 *
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL
#define UBRR_VALOR 103

#include "uart.h"

/****************************************/
// Function prototypes

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines

void uart_init(void)
{
	//Velocidad de 9600 baudios
	UBRR0H = (uint8_t)(UBRR_VALOR >> 8);
	UBRR0L = (uint8_t)UBRR_VALOR;

	//Activar RX y TX
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	//8 bits de datos
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char dato)
{
	//Esperar buffer libre
	while (!(UCSR0A & (1 << UDRE0)))
	{
	}

	//Mandar un caracter
	UDR0 = dato;
}

void uart_print(const char *txt)
{
	//Mandar texto por UART
	while (*txt != '\0')
	{
		uart_tx(*txt);
		txt++;
	}
}

uint8_t uart_hay_dato(void)
{
	//Ver si llego un dato
	if (UCSR0A & (1 << RXC0))
	{
		return 1;
	}

	return 0;
}

char uart_rx(void)
{
	//Esperar dato recibido
	while (!(UCSR0A & (1 << RXC0)))
	{
	}

	//Devolver el dato
	return UDR0;
}

/****************************************/
// Interrupt routines
