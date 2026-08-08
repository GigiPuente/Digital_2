/*
 * main_maestro.c
 *
 * Maestro I2C con LCD 16x2 sin modulo.
 * Conexiones:
 * RS D4, RW D3, E D2
 * D0 D12, D1 D11, D2 D10, D3 D9,
 * D4 D8, D5 D7, D6 D6, D7 D5
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
#define LCD_RW_BIT   PD3
#define LCD_RS_BIT   PD4
#define LCD_D7_BIT   PD5
#define LCD_D6_BIT   PD6
#define LCD_D5_BIT   PD7

#define LCD_D4_BIT   PB0
#define LCD_D3_BIT   PB1
#define LCD_D2_BIT   PB2
#define LCD_D1_BIT   PB3
#define LCD_D0_BIT   PB4

#define I2C_SLAVE_ADDRESS        0x12U

#define TWI_START_STATUS         0x08U
#define TWI_REP_START_STATUS     0x10U
#define TWI_MT_SLA_ACK_STATUS    0x18U
#define TWI_MT_DATA_ACK_STATUS   0x28U
#define TWI_MR_SLA_ACK_STATUS    0x40U
#define TWI_MR_DATA_ACK_STATUS   0x50U
#define TWI_MR_DATA_NACK_STATUS  0x58U

typedef struct {
    uint8_t proximity_active;
    uint8_t servo_state;
    uint8_t servo_angle;
} slave_status_t;

static slave_status_t g_slave = {0U, 0U, 0U};
static uint16_t g_box_count = 0U;
static uint8_t g_prev_proximity = 0U;

static void gpio_init(void)
{
    DDRD |= (1 << LCD_E_BIT) | (1 << LCD_RW_BIT) | (1 << LCD_RS_BIT) |
            (1 << LCD_D7_BIT) | (1 << LCD_D6_BIT) | (1 << LCD_D5_BIT);
    DDRB |= (1 << LCD_D4_BIT) | (1 << LCD_D3_BIT) | (1 << LCD_D2_BIT) |
            (1 << LCD_D1_BIT) | (1 << LCD_D0_BIT);

    PORTD &= ~((1 << LCD_E_BIT) | (1 << LCD_RW_BIT) | (1 << LCD_RS_BIT) |
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

    PORTD &= ~(1 << LCD_RW_BIT);
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
    PORTD &= ~(1 << LCD_RW_BIT);
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
    TWSR = 0x00U;
    TWBR = (uint8_t)(((F_CPU / 100000UL) - 16UL) / 2UL);
    TWCR = (1 << TWEN);
}

static bool twi_wait_interrupt(void)
{
    uint16_t timeout = 65535U;

    while (((TWCR & (1 << TWINT)) == 0U) && (timeout > 0U)) {
        timeout--;
    }

    return (timeout > 0U);
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

static bool read_slave_registers(uint8_t start_reg, uint8_t *buffer, uint8_t length)
{
    uint8_t i;

    if (!twi_start((uint8_t)(I2C_SLAVE_ADDRESS << 1))) {
        return false;
    }

    if (!twi_write(start_reg)) {
        twi_stop();
        return false;
    }

    if (!twi_start((uint8_t)((I2C_SLAVE_ADDRESS << 1) | 1U))) {
        twi_stop();
        return false;
    }

    for (i = 0U; i < length; i++) {
        if (!twi_read(&buffer[i], i < (uint8_t)(length - 1U))) {
            twi_stop();
            return false;
        }
    }

    twi_stop();
    return true;
}

static void poll_slave(void)
{
    uint8_t data[4];

    if (read_slave_registers(0U, data, sizeof(data))) {
        g_slave.proximity_active = data[1];
        g_slave.servo_state = data[2];
        g_slave.servo_angle = data[3];
    }
}

static void update_box_counter(void)
{
    if (g_slave.proximity_active && !g_prev_proximity) {
        g_box_count++;
    }

    g_prev_proximity = g_slave.proximity_active;
}

static const char *servo_text(uint8_t state)
{
    switch (state) {
    case 1U: return "WAIT";
    case 2U: return "90G ";
    default: return "IDLE";
    }
}

static void draw_screen(void)
{
    char line[17];

    lcd_set_cursor(0U, 0U);
    lcd_print_padded("Cantidad cajas");

    lcd_set_cursor(0U, 1U);
    snprintf(line, sizeof(line), "%5u %s %3u",
             g_box_count,
             servo_text(g_slave.servo_state),
             g_slave.servo_angle);
    lcd_print_padded(line);
}

int main(void)
{
    gpio_init();
    lcd_init();

    lcd_set_cursor(0U, 0U);
    lcd_print_padded("Cantidad cajas");
    lcd_set_cursor(0U, 1U);
    lcd_print_padded("Iniciando...");
    _delay_ms(2000);

    twi_init();

    for (;;) {
        poll_slave();
        update_box_counter();
        draw_screen();
        _delay_ms(200);
    }
}
