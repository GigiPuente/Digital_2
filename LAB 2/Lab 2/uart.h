#ifndef UART_H_
#define UART_H_

#include <avr/io.h>

void uart_init(void);
void uart_tx(char dato);
void uart_print(const char *txt);
uint8_t uart_hay_dato(void);
char uart_rx(void);

#endif /* UART_H_ */
