/*
 * main_maestro.c
 *
 * Maestro I2C con LCD 16x2:
 * - Lee solo el contador de cajas del esclavo
 * - Muestra "Cantidad cajas" y el valor en la segunda fila
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define LCD_E_BIT    PD2
#define LCD_RS_BIT   PD4
#define LCD_D7_BIT   PD5
#define LCD_D6_BIT   PD6
#define LCD_D5_BIT   PD7

#define LCD_D4_BIT   PB0
#define LCD_D3_BIT   PB1
#define LCD_D2_BIT   PB2
#define LCD_D1_BIT   PB3
#define LCD_D0_BIT   PB4

#define TWI_SDA_BIT  PC4
#define TWI_SCL_BIT  PC5

#define I2C_SLAVE_ADDRESS        0x12U
#define LM75_I2C_ADDRESS         0x48U
#define LM75_TEMP_REGISTER       0x00U
#define TEMP_STOP_THRESHOLD_C    80
#define SLAVE_CMD_PAUSE_5S       0xA5U
#define I2C_PRESCALER_BITS       0x03U
#define I2C_BITRATE_VALUE        12U
#define TELEMETRY_INTERVAL_MS    1000UL
#define SOFT_UART_BIT_US         104U

#define TWI_START_STATUS         0x08U
#define TWI_REP_START_STATUS     0x10U
#define TWI_MT_SLA_ACK_STATUS    0x18U
#define TWI_MT_DATA_ACK_STATUS   0x28U
#define TWI_MR_SLA_ACK_STATUS    0x40U
#define TWI_MR_DATA_ACK_STATUS   0x50U
#define TWI_MR_DATA_NACK_STATUS  0x58U

static uint16_t g_box_count = 0U;
static uint8_t g_i2c_online = 0U;
static uint8_t g_overtemp_latched = 0U;
static int8_t g_last_temp_c = 0;
static uint8_t g_temp_valid = 0U;

static void telemetry_uart_init(void)
{
    DDRD |= (1 << PD3);
    PORTD |= (1 << PD3);
}

static void telemetry_uart_write_byte(uint8_t value)
{
    uint8_t bit_index;

    PORTD &= ~(1 << PD3);
    _delay_us(SOFT_UART_BIT_US);

    for (bit_index = 0U; bit_index < 8U; bit_index++) {
        if ((value & (1U << bit_index)) != 0U) {
            PORTD |= (1 << PD3);
        } else {
            PORTD &= ~(1 << PD3);
        }
        _delay_us(SOFT_UART_BIT_US);
    }

    PORTD |= (1 << PD3);
    _delay_us(SOFT_UART_BIT_US);
}

static void telemetry_uart_write_text(const char *text)
{
    while (*text != '\0') {
        telemetry_uart_write_byte((uint8_t)(*text));
        text++;
    }
}

static void telemetry_send_frame(void)
{
    char line[64];

    snprintf(line,
             sizeof(line),
             "boxes=%u,temp_c=%d,alarm=%u,i2c=%u\r\n",
             g_box_count,
             g_temp_valid ? g_last_temp_c : -127,
             g_overtemp_latched,
             g_i2c_online);
    telemetry_uart_write_text(line);
}

static void gpio_init(void)
{
    DDRD |= (1 << LCD_E_BIT) | (1 << LCD_RS_BIT) |
            (1 << LCD_D7_BIT) | (1 << LCD_D6_BIT) | (1 << LCD_D5_BIT);
    DDRB |= (1 << LCD_D4_BIT) | (1 << LCD_D3_BIT) | (1 << LCD_D2_BIT) |
            (1 << LCD_D1_BIT) | (1 << LCD_D0_BIT);

    PORTD &= ~((1 << LCD_E_BIT) | (1 << LCD_RS_BIT) |
               (1 << LCD_D7_BIT) | (1 << LCD_D6_BIT) | (1 << LCD_D5_BIT));
    PORTB &= ~((1 << LCD_D4_BIT) | (1 << LCD_D3_BIT) | (1 << LCD_D2_BIT) |
               (1 << LCD_D1_BIT) | (1 << LCD_D0_BIT));
}

static void lcd_set_data(uint8_t value)
{
    if (value & (1U << 0)) { PORTB |= (1 << LCD_D0_BIT); } else { PORTB &= ~(1 << LCD_D0_BIT); }
    if (value & (1U << 1)) { PORTB |= (1 << LCD_D1_BIT); } else { PORTB &= ~(1 << LCD_D1_BIT); }
    if (value & (1U << 2)) { PORTB |= (1 << LCD_D2_BIT); } else { PORTB &= ~(1 << LCD_D2_BIT); }
    if (value & (1U << 3)) { PORTB |= (1 << LCD_D3_BIT); } else { PORTB &= ~(1 << LCD_D3_BIT); }
    if (value & (1U << 4)) { PORTB |= (1 << LCD_D4_BIT); } else { PORTB &= ~(1 << LCD_D4_BIT); }
    if (value & (1U << 5)) { PORTD |= (1 << LCD_D5_BIT); } else { PORTD &= ~(1 << LCD_D5_BIT); }
    if (value & (1U << 6)) { PORTD |= (1 << LCD_D6_BIT); } else { PORTD &= ~(1 << LCD_D6_BIT); }
    if (value & (1U << 7)) { PORTD |= (1 << LCD_D7_BIT); } else { PORTD &= ~(1 << LCD_D7_BIT); }
}

static void lcd_pulse_enable(void)
{
    PORTD |= (1 << LCD_E_BIT);
    _delay_us(1);
    PORTD &= ~(1 << LCD_E_BIT);
    _delay_us(50);
}

static void lcd_write_raw(uint8_t value, uint8_t rs)
{
    if (rs) {
        PORTD |= (1 << LCD_RS_BIT);
    } else {
        PORTD &= ~(1 << LCD_RS_BIT);
    }

    lcd_set_data(value);
    lcd_pulse_enable();
}

static void lcd_command(uint8_t command)
{
    lcd_write_raw(command, 0U);
    _delay_ms(2);
}

static void lcd_data(uint8_t value)
{
    lcd_write_raw(value, 1U);
}

static void lcd_clear(void)
{
    lcd_command(0x01U);
    _delay_ms(2);
}

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    const uint8_t base = (row == 0U) ? 0x00U : 0x40U;
    lcd_command((uint8_t)(0x80U | (base + col)));
}

static void lcd_print_padded(const char *text)
{
    uint8_t i = 0U;

    while ((text[i] != '\0') && (i < 16U)) {
        lcd_data((uint8_t)text[i]);
        i++;
    }

    while (i < 16U) {
        lcd_data(' ');
        i++;
    }
}

static void lcd_init(void)
{
    PORTD &= ~(1 << LCD_RS_BIT);
    PORTD &= ~(1 << LCD_E_BIT);

    _delay_ms(50);

    lcd_set_data(0x30U);
    lcd_pulse_enable();
    _delay_ms(5);

    lcd_set_data(0x30U);
    lcd_pulse_enable();
    _delay_ms(1);

    lcd_set_data(0x30U);
    lcd_pulse_enable();
    _delay_ms(1);

    lcd_command(0x38U);
    lcd_command(0x0CU);
    lcd_command(0x06U);
    lcd_clear();
}

static void twi_init(void)
{
    DDRC &= ~((1 << TWI_SDA_BIT) | (1 << TWI_SCL_BIT));
    PORTC |= (1 << TWI_SDA_BIT) | (1 << TWI_SCL_BIT);

    TWSR = I2C_PRESCALER_BITS;
    TWBR = I2C_BITRATE_VALUE;
    TWCR = (1 << TWEN);
}

static bool twi_wait_interrupt(void)
{
    uint32_t timeout_us = 200000UL;

    while (((TWCR & (1 << TWINT)) == 0U) && (timeout_us > 0UL)) {
        _delay_us(1);
        timeout_us--;
    }

    return (timeout_us > 0UL);
}

static uint8_t twi_status(void)
{
    return (uint8_t)(TWSR & 0xF8U);
}

static bool twi_start(uint8_t address_rw)
{
    uint8_t status;

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (!twi_wait_interrupt()) {
        return false;
    }

    status = twi_status();
    if ((status != TWI_START_STATUS) && (status != TWI_REP_START_STATUS)) {
        return false;
    }

    TWDR = address_rw;
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (!twi_wait_interrupt()) {
        return false;
    }

    status = twi_status();
    if ((address_rw & 1U) == 0U) {
        return (status == TWI_MT_SLA_ACK_STATUS);
    }

    return (status == TWI_MR_SLA_ACK_STATUS);
}

static bool twi_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!twi_wait_interrupt()) {
        return false;
    }

    return (twi_status() == TWI_MT_DATA_ACK_STATUS);
}

static bool twi_read(uint8_t *data, bool acknowledge)
{
    if (acknowledge) {
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    } else {
        TWCR = (1 << TWINT) | (1 << TWEN);
    }

    if (!twi_wait_interrupt()) {
        return false;
    }

    if (acknowledge) {
        if (twi_status() != TWI_MR_DATA_ACK_STATUS) {
            return false;
        }
    } else if (twi_status() != TWI_MR_DATA_NACK_STATUS) {
        return false;
    }

    *data = TWDR;
    return true;
}

static void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

static bool write_slave_command(uint8_t command)
{
    if (!twi_start((uint8_t)(I2C_SLAVE_ADDRESS << 1))) {
        twi_stop();
        return false;
    }

    if (!twi_write(command)) {
        twi_stop();
        return false;
    }

    twi_stop();
    return true;
}

static bool read_slave_box_count(uint8_t *count)
{
    uint8_t data;
    uint8_t attempts;

    for (attempts = 0U; attempts < 3U; attempts++) {
        if (!twi_start((uint8_t)((I2C_SLAVE_ADDRESS << 1) | 1U))) {
            twi_stop();
            _delay_ms(5);
            continue;
        }

        if (!twi_read(&data, false)) {
            twi_stop();
            _delay_ms(5);
            continue;
        }

        twi_stop();
        *count = data;
        return true;
    }

    return false;
}

static bool read_lm75_temp_c(int8_t *temp_c)
{
    uint8_t msb;
    uint8_t lsb;

    if (!twi_start((uint8_t)(LM75_I2C_ADDRESS << 1))) {
        twi_stop();
        return false;
    }

    if (!twi_write(LM75_TEMP_REGISTER)) {
        twi_stop();
        return false;
    }

    if (!twi_start((uint8_t)((LM75_I2C_ADDRESS << 1) | 1U))) {
        twi_stop();
        return false;
    }

    if (!twi_read(&msb, true)) {
        twi_stop();
        return false;
    }

    if (!twi_read(&lsb, false)) {
        twi_stop();
        return false;
    }

    twi_stop();
    (void)lsb;
    *temp_c = (int8_t)msb;
    return true;
}

static void draw_screen(void)
{
    char line[17];

    lcd_set_cursor(0U, 0U);
    lcd_print_padded("Cantidad cajas");

    lcd_set_cursor(0U, 1U);
    if (g_i2c_online) {
        snprintf(line, sizeof(line), "%u", g_box_count);
    } else {
        snprintf(line, sizeof(line), "I2C sin ACK");
    }
    lcd_print_padded(line);
}

int main(void)
{
    gpio_init();
    lcd_init();
    twi_init();
    telemetry_uart_init();

    lcd_set_cursor(0U, 0U);
    lcd_print_padded("Cantidad cajas");
    lcd_set_cursor(0U, 1U);
    lcd_print_padded("I2C sin ACK");
    _delay_ms(500);

    for (;;) {
        uint8_t box_count;
        int8_t temp_c;
        static uint32_t telemetry_elapsed_ms = 0UL;

        if (read_lm75_temp_c(&temp_c)) {
            g_last_temp_c = temp_c;
            g_temp_valid = 1U;
            if ((temp_c >= TEMP_STOP_THRESHOLD_C) && !g_overtemp_latched) {
                write_slave_command(SLAVE_CMD_PAUSE_5S);
                g_overtemp_latched = 1U;
            } else if (temp_c < TEMP_STOP_THRESHOLD_C) {
                g_overtemp_latched = 0U;
            }
        } else {
            g_temp_valid = 0U;
        }

        if (read_slave_box_count(&box_count)) {
            g_box_count = box_count;
            g_i2c_online = 1U;
        } else {
            g_i2c_online = 0U;
        }

        draw_screen();
        telemetry_elapsed_ms += 200UL;
        if (telemetry_elapsed_ms >= TELEMETRY_INTERVAL_MS) {
            telemetry_elapsed_ms = 0UL;
            telemetry_send_frame();
        }
        _delay_ms(200);
    }
}
