/*
 * uart.c
 *
 * Created: 
 * Author: 
 * Description: 
 * 
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL
#define UBRR_VALOR 103
#define UART_BUFFER 32

#include <avr/interrupt.h>
#include "uart.h"

volatile char rx_buffer[UART_BUFFER];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

/****************************************/
// Function prototypes

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines

void uart_init(void)
{
	UBRR0H = (uint8_t)(UBRR_VALOR >> 8);
	UBRR0L = (uint8_t)UBRR_VALOR;

	// Activar RX, TX e interrupcion RX
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

	// 8 bits de datos
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

	sei();
}

void uart_tx(char dato)
{
	while (!(UCSR0A & (1 << UDRE0)))
	{
	}

	UDR0 = dato;
}

void uart_print(const char *txt)
{
	while (*txt != '\0')
	{
		uart_tx(*txt);
		txt++;
	}
}

uint8_t uart_hay_dato(void)
{
	if (rx_head != rx_tail)
	{
		return 1;
	}

	return 0;
}

char uart_rx(void)
{
	char dato;

	while (rx_head == rx_tail)
	{
	}

	dato = rx_buffer[rx_tail];
	rx_tail++;

	if (rx_tail >= UART_BUFFER)
	{
		rx_tail = 0;
	}

	return dato;
}

/****************************************/
// Interrupt routines

ISR(USART_RX_vect)
{
	uint8_t siguiente = rx_head + 1;

	if (siguiente >= UART_BUFFER)
	{
		siguiente = 0;
	}

	if (siguiente != rx_tail)
	{
		rx_buffer[rx_head] = UDR0;
		rx_head = siguiente;
	}
	else
	{
		// Leer el dato aunque el buffer este lleno
		char basura = UDR0;
		(void)basura;
	}
}
