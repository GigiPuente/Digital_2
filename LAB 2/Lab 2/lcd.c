/*
 * lcd.c
 *
 * Created: 16/07/2026
 * Author: Jorge Puente
 * Description: 
 * 
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL

#include "lcd.h"
#include <util/delay.h>

#define LCD_RS_PORT PORTC
#define LCD_RS_DDR  DDRC
#define LCD_RS  PC2

#define LCD_E_PORT  PORTC
#define LCD_E_DDR   DDRC
#define LCD_E   PC3

#define LCD_D0_PORT PORTC
#define LCD_D0_DDR  DDRC
#define LCD_D0  PC1

#define LCD_D1_PORT PORTC
#define LCD_D1_DDR  DDRC
#define LCD_D1  PC0

#define LCD_D2_PORT PORTB
#define LCD_D2_DDR  DDRB
#define LCD_D2  PB5

#define LCD_D3_PORT PORTB
#define LCD_D3_DDR  DDRB
#define LCD_D3  PB4

#define LCD_D4_PORT PORTB
#define LCD_D4_DDR  DDRB
#define LCD_D4  PB3

#define LCD_D5_PORT PORTB
#define LCD_D5_DDR  DDRB
#define LCD_D5  PB2

#define LCD_D6_PORT PORTB
#define LCD_D6_DDR  DDRB
#define LCD_D6  PB1

#define LCD_D7_PORT PORTB
#define LCD_D7_DDR  DDRB
#define LCD_D7  PB0

/****************************************/
// Function prototypes

static void lcd_write(uint8_t dato);
static void lcd_enable(void);

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines

void lcd_init(void)
{
	//Pines del LCD como salida
	LCD_RS_DDR |= (1 << LCD_RS);
	LCD_E_DDR |= (1 << LCD_E);
	LCD_D0_DDR |= (1 << LCD_D0);
	LCD_D1_DDR |= (1 << LCD_D1);
	LCD_D2_DDR |= (1 << LCD_D2);
	LCD_D3_DDR |= (1 << LCD_D3);
	LCD_D4_DDR |= (1 << LCD_D4);
	LCD_D5_DDR |= (1 << LCD_D5);
	LCD_D6_DDR |= (1 << LCD_D6);
	LCD_D7_DDR |= (1 << LCD_D7);

	//Esperar a que encienda bien
	_delay_ms(15);

	//Modo 8 bits (2 lineas)
	lcd(0x38);

	_delay_us(100);

	//Display encendido
	lcd(0x0C);

	_delay_us(100);

	//El cursor avanza a la derecha
	lcd(0x06);

	_delay_us(100);

	//Limpiar la pantalla
	lcd(0x01);
	_delay_ms(2);
}

void lcd(uint8_t dato)
{
	//RS en 0 para mandar datos
	LCD_RS_PORT &= ~(1 << LCD_RS);
	//Mandar el dato a los pines
	lcd_write(dato);
	//Enable
	lcd_enable();
	_delay_ms(2);
}

void lcd_data(uint8_t dato)
{
	//RS en 1 para mandar dato
	LCD_RS_PORT |= (1 << LCD_RS);
	//Mandar dato a los pines
	lcd_write(dato);
	//Enable
	lcd_enable();
	_delay_ms(2);
}

void lcd_print(const char *txt)
{
	//Manda letra por letra al LCD
	while (*txt != '\0')
	{
		lcd_data((uint8_t)*txt);
		txt++;
	}
}

void lcd_posicion(uint8_t fila, uint8_t col)
{
	//Guarda la direccion de la posicion
	uint8_t direccion = (fila == 0) ? 0x00 : 0x40;
	direccion += col;
	//Manda la direccion al LCD
	lcd(0x80 | direccion);
}

static void lcd_write(uint8_t dato)
{
	//Cada if pone un bit en su pin
	if (dato & (1 << 0)) LCD_D0_PORT |= (1 << LCD_D0);
	else LCD_D0_PORT &= ~(1 << LCD_D0);

	if (dato & (1 << 1)) LCD_D1_PORT |= (1 << LCD_D1);
	else LCD_D1_PORT &= ~(1 << LCD_D1);

	if (dato & (1 << 2)) LCD_D2_PORT |= (1 << LCD_D2);
	else LCD_D2_PORT &= ~(1 << LCD_D2);

	if (dato & (1 << 3)) LCD_D3_PORT |= (1 << LCD_D3);
	else LCD_D3_PORT &= ~(1 << LCD_D3);

	if (dato & (1 << 4)) LCD_D4_PORT |= (1 << LCD_D4);
	else LCD_D4_PORT &= ~(1 << LCD_D4);

	if (dato & (1 << 5)) LCD_D5_PORT |= (1 << LCD_D5);
	else LCD_D5_PORT &= ~(1 << LCD_D5);

	if (dato & (1 << 6)) LCD_D6_PORT |= (1 << LCD_D6);
	else LCD_D6_PORT &= ~(1 << LCD_D6);

	if (dato & (1 << 7)) LCD_D7_PORT |= (1 << LCD_D7);
	else LCD_D7_PORT &= ~(1 << LCD_D7);
}

static void lcd_enable(void)
{
	//Pulso para que el LCD pueda leer el dato
	LCD_E_PORT |= (1 << LCD_E);
	_delay_us(1);
	LCD_E_PORT &= ~(1 << LCD_E);
	_delay_us(100);
}

/****************************************/
// Interrupt routines
