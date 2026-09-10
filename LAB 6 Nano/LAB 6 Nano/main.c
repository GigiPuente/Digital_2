/*
 * main.c
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
#include <stdint.h>
#include <util/delay.h>

//Baudrate 
#define BAUD 9600UL

//Calculo Double Speed 
#define UBRR_VALUE ((F_CPU + (4UL * BAUD)) / (8UL * BAUD) - 1UL)

// Botones 
#define BTN_ARRIBA    (1U << PD2)
#define BTN_ABAJO     (1U << PD3)
#define BTN_IZQUIERDA (1U << PD4)
#define BTN_DERECHA   (1U << PD5)
#define BTN_A         (1U << PD6)
#define BTN_B         (1U << PD7)

#define BTN_MASK (BTN_ARRIBA    | BTN_ABAJO     | \
                  BTN_IZQUIERDA | BTN_DERECHA   | \
                  BTN_A         | BTN_B)

/****************************************/
// Function prototypes

void UART_Init(uint16_t ubrr);
void UART_TransmitChar(char data);
void UART_TransmitString(const char *str);

/****************************************/
// Main Function

int main(void)
{
    uint8_t estado_anterior;

    // PD2-PD7 entradas
    DDRD &= (uint8_t)~BTN_MASK;

    // Pull-up
    PORTD |= BTN_MASK;

    //Inicializar UART
    UART_Init((uint16_t)UBRR_VALUE);


    _delay_ms(100);

    // Mensaje
    UART_TransmitString("Nano iniciado\r\n");


    estado_anterior = BTN_MASK;

    while (1)
    {
        uint8_t lectura = PIND & BTN_MASK;

        if (lectura != estado_anterior)
        {
            //Antirrebote
            _delay_ms(20);

            uint8_t lectura_confirmada = PIND & BTN_MASK;

            //Detectar flancos
            uint8_t botones_presionados =
                estado_anterior &
                (uint8_t)~lectura_confirmada &
                BTN_MASK;

            if (botones_presionados & BTN_ARRIBA)
            {
                UART_TransmitString("Arriba\r\n");
            }

            if (botones_presionados & BTN_ABAJO)
            {
                UART_TransmitString("Abajo\r\n");
            }

            if (botones_presionados & BTN_IZQUIERDA)
            {
                UART_TransmitString("Izquierda\r\n");
            }

            if (botones_presionados & BTN_DERECHA)
            {
                UART_TransmitString("Derecha\r\n");
            }

            if (botones_presionados & BTN_A)
            {
                UART_TransmitString("A\r\n");
            }

            if (botones_presionados & BTN_B)
            {
                UART_TransmitString("B\r\n");
            }

            // Guardar estados
            estado_anterior = lectura_confirmada;
        }
    }
}

/****************************************/
// NON-Interrupt subroutines

void UART_Init(uint16_t ubrr)
{
 
    UCSR0A = (1U << U2X0);

    // Cargar el divisor
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Habilitar el transmisor 
    UCSR0B = (1U << TXEN0);

    UCSR0C = (1U << UCSZ01) | (1U << UCSZ00);
}

void UART_TransmitChar(char data)
{
    //Esperar
    while (!(UCSR0A & (1U << UDRE0)))
    {
    }

    UDR0 = (uint8_t)data;
}

void UART_TransmitString(const char *str)
{
    while (*str != '\0')
    {
        UART_TransmitChar(*str);
        str++;
    }
}

/****************************************/
// Interrupt routines

// No se utilizan interrupciones en el ATmega328P.
