#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>

void lcd_init(void);
void lcd(uint8_t command);
void lcd_data(uint8_t data);
void lcd_print(const char *text);
void lcd_posicion(uint8_t row, uint8_t column);

#endif /* LCD_H_ */
