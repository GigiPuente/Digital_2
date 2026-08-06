#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

// Mapeo de pines:

#define LCD_RS_PUERTO PORTD
#define LCD_RS_DDR    DDRD
#define LCD_RS_BIT    PD4

#define LCD_RW_PUERTO PORTD
#define LCD_RW_DDR    DDRD
#define LCD_RW_BIT    PD3

#define LCD_E_PUERTO  PORTD
#define LCD_E_DDR     DDRD
#define LCD_E_BIT     PD2

#define CANAL_ADC 4
#define UMBRAL_MIN_ADC 20
#define UMBRAL_MAX_ADC 1000
#define MAXIMO_CAJAS 9999
#define BOTON_RESET_DDR DDRC
#define BOTON_RESET_PUERTO PORTC
#define BOTON_RESET_LECTURA PINC
#define BOTON_RESET_BIT PC5

static void lcd_escribir_bus(uint8_t valor)
{
    // D0..D4 del LCD conectados en PB4..PB0
    PORTB = (PORTB & ~((1 << PB4) | (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0)))
          | ((valor & (1 << 0)) ? (1 << PB4) : 0)
          | ((valor & (1 << 1)) ? (1 << PB3) : 0)
          | ((valor & (1 << 2)) ? (1 << PB2) : 0)
          | ((valor & (1 << 3)) ? (1 << PB1) : 0)
          | ((valor & (1 << 4)) ? (1 << PB0) : 0);

    // D5..D7 del LCD conectados en PD7..PD5
    PORTD = (PORTD & ~((1 << PD7) | (1 << PD6) | (1 << PD5)))
          | ((valor & (1 << 5)) ? (1 << PD7) : 0)
          | ((valor & (1 << 6)) ? (1 << PD6) : 0)
          | ((valor & (1 << 7)) ? (1 << PD5) : 0);
}

static void lcd_pulso_enable(void)
{
    LCD_E_PUERTO |= (1 << LCD_E_BIT);
    _delay_us(1);
    LCD_E_PUERTO &= ~(1 << LCD_E_BIT);
    _delay_us(100);
}

static void lcd_comando(uint8_t comando)
{
    LCD_RS_PUERTO &= ~(1 << LCD_RS_BIT);
    LCD_RW_PUERTO &= ~(1 << LCD_RW_BIT);
    lcd_escribir_bus(comando);
    lcd_pulso_enable();
    _delay_ms(2);
}

static void lcd_dato(uint8_t dato)
{
    LCD_RS_PUERTO |= (1 << LCD_RS_BIT);
    LCD_RW_PUERTO &= ~(1 << LCD_RW_BIT);
    lcd_escribir_bus(dato);
    lcd_pulso_enable();
    _delay_us(50);
}

static void lcd_inicializar(void)
{
    DDRB |= (1 << PB4) | (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);
    DDRD |= (1 << PD7) | (1 << PD6) | (1 << PD5);

    LCD_RS_DDR |= (1 << LCD_RS_BIT);
    LCD_RW_DDR |= (1 << LCD_RW_BIT);
    LCD_E_DDR  |= (1 << LCD_E_BIT);

    _delay_ms(20);
    lcd_comando(0x38); // Modo 8 bits, 2 lineas
    lcd_comando(0x0C); // Pantalla encendida, cursor apagado
    lcd_comando(0x01); // Limpiar pantalla
    _delay_ms(2);
    lcd_comando(0x06); // Modo de entrada
}

static void lcd_posicionar_cursor(uint8_t fila, uint8_t columna)
{
    uint8_t direccion = (fila == 0) ? columna : (0x40 + columna);
    lcd_comando(0x80 | direccion);
}

static void lcd_imprimir(const char *texto)
{
    while (*texto != '\0')
    {
        lcd_dato((uint8_t)*texto);
        texto++;
    }
}

static void lcd_imprimir_uint16(uint16_t valor)
{
    char digitos[5];
    uint8_t indice = 0;

    if (valor == 0)
    {
        lcd_dato('0');
        return;
    }

    while (valor > 0)
    {
        digitos[indice] = (char)('0' + (valor % 10));
        valor /= 10;
        indice++;
    }

    while (indice > 0)
    {
        indice--;
        lcd_dato((uint8_t)digitos[indice]);
    }
}

static void lcd_mostrar_cajas(uint16_t cajas)
{
    lcd_posicionar_cursor(1, 0);
    lcd_imprimir("                ");
    lcd_posicionar_cursor(1, 0);
    lcd_imprimir_uint16(cajas);
}

static void adc_inicializar(void)
{
    ADMUX = (1 << REFS0) | CANAL_ADC;
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static uint16_t adc_leer(void)
{
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}

static void boton_reset_inicializar(void)
{
    BOTON_RESET_DDR &= ~(1 << BOTON_RESET_BIT);
    BOTON_RESET_PUERTO |= (1 << BOTON_RESET_BIT);
}

static uint8_t boton_reset_presionado(void)
{
    if ((BOTON_RESET_LECTURA & (1 << BOTON_RESET_BIT)) == 0)
    {
        _delay_ms(25);
        if ((BOTON_RESET_LECTURA & (1 << BOTON_RESET_BIT)) == 0)
        {
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    uint16_t valor_adc;
    uint16_t cajas_actuales = 0;
    uint8_t extremo_liberado = 1;

    lcd_inicializar();
    adc_inicializar();
    boton_reset_inicializar();

    lcd_posicionar_cursor(0, 0);
    lcd_imprimir("cantidad cajas");
    lcd_mostrar_cajas(cajas_actuales);

    while (1)
    {
        if (boton_reset_presionado())
        {
            cajas_actuales = 0;
            lcd_mostrar_cajas(cajas_actuales);

            while ((BOTON_RESET_LECTURA & (1 << BOTON_RESET_BIT)) == 0)
            {
            }

            _delay_ms(25);
            extremo_liberado = 1;
        }

        valor_adc = adc_leer();

        if ((valor_adc >= UMBRAL_MAX_ADC) && extremo_liberado)
        {
            if (cajas_actuales < MAXIMO_CAJAS)
            {
                cajas_actuales++;
            }
            extremo_liberado = 0;
            lcd_mostrar_cajas(cajas_actuales);
        }
        else if ((valor_adc <= UMBRAL_MIN_ADC) && extremo_liberado)
        {
            if (cajas_actuales < MAXIMO_CAJAS)
            {
                cajas_actuales++;
            }
            extremo_liberado = 0;
            lcd_mostrar_cajas(cajas_actuales);
        }
        else if ((valor_adc > UMBRAL_MIN_ADC) && (valor_adc < UMBRAL_MAX_ADC))
        {
            extremo_liberado = 1;
        }

        _delay_ms(150);
    }
}
