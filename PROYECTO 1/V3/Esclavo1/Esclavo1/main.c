/*
 * main.c
 *
 * Esclavo I2C simplificado:
 * - Detecta proximidad
 * - Mueve el servo
 * - Cuenta una caja cada vez que el servo llega a 90 grados
 * - Expone el contador por I2C
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#define PROX_BIT          PB2
#define DC_AIN2_BIT       PD6
#define DC_AIN1_BIT       PD7
#define SERVO_BIT         PB1
#define DC_PWMA_BIT       PB0

#define TWI_SDA_BIT       PC4
#define TWI_SCL_BIT       PC5

#define PROX_ACTIVE_LOW   0
#define SERVO_WAIT_MS     1000UL
#define SERVO_HOLD_MS     1000UL
#define DC_STOP_MS        3000UL
#define SYSTEM_PAUSE_MS   5000UL

#define I2C_SLAVE_ADDRESS 0x12U
#define I2C_PRESCALER_BITS 0x03U
#define I2C_BITRATE_VALUE  12U
#define SLAVE_CMD_PAUSE_5S 0xA5U

static volatile uint32_t g_millis = 0UL;
static volatile uint8_t g_i2c_box_count = 0U;
static volatile uint8_t g_i2c_rx_command = 0U;
static uint8_t g_box_count = 0U;

typedef struct {
    uint8_t stable_state;
    uint8_t last_raw_state;
    uint32_t last_change_ms;
} debounce_t;

typedef enum {
    SERVO_IDLE = 0,
    SERVO_WAITING,
    SERVO_AT_90
} servo_state_t;

static servo_state_t servo_state = SERVO_IDLE;
static uint32_t servo_deadline_ms = 0UL;
static uint8_t g_servo_position_deg = 0U;
static uint32_t g_dc_resume_ms = 0UL;
static uint8_t g_dc_running = 0U;
static uint8_t g_system_paused = 0U;
static uint32_t g_system_pause_until_ms = 0UL;

ISR(TIMER0_COMPA_vect)
{
    g_millis++;
}

ISR(TWI_vect)
{
    switch (TWSR & 0xF8U) {
    case 0x60U:
    case 0x68U:
    case 0x70U:
    case 0x78U:
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0x80U:
    case 0x90U:
        g_i2c_rx_command = TWDR;
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0xA0U:
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0xA8U:
        TWDR = g_i2c_box_count;
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0xC0U:
    case 0xC8U:
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    default:
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;
    }
}

static uint32_t millis_get(void)
{
    uint32_t value;
    uint8_t old_sreg = SREG;

    cli();
    value = g_millis;
    SREG = old_sreg;
    return value;
}

static bool time_reached(uint32_t now, uint32_t deadline)
{
    return ((int32_t)(now - deadline) >= 0);
}

static bool interval_elapsed(uint32_t now, uint32_t previous, uint32_t interval)
{
    return ((uint32_t)(now - previous) >= interval);
}

static void timer0_millis_init(void)
{
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 249;
    TIMSK0 = (1 << OCIE0A);
}

static uint8_t proximity_active_raw(void)
{
#if PROX_ACTIVE_LOW
    return ((PINB & (1 << PROX_BIT)) == 0U);
#else
    return ((PINB & (1 << PROX_BIT)) != 0U);
#endif
}

static void gpio_init(void)
{
    DDRB &= ~(1 << PROX_BIT);
    PORTB |= (1 << PROX_BIT);
    DDRD |= (1 << DC_AIN1_BIT) | (1 << DC_AIN2_BIT);
    PORTD &= ~((1 << DC_AIN1_BIT) | (1 << DC_AIN2_BIT));

    DDRB |= (1 << DC_PWMA_BIT);
    PORTB &= ~(1 << DC_PWMA_BIT);

    DDRC &= ~((1 << TWI_SDA_BIT) | (1 << TWI_SCL_BIT));
    PORTC |= (1 << TWI_SDA_BIT) | (1 << TWI_SCL_BIT);
}

static void debounce_init(debounce_t *input, uint8_t initial_state, uint32_t now)
{
    input->stable_state = initial_state;
    input->last_raw_state = initial_state;
    input->last_change_ms = now;
}

static bool debounce_update(debounce_t *input, uint8_t raw_state, uint32_t now)
{
    if (raw_state != input->last_raw_state) {
        input->last_raw_state = raw_state;
        input->last_change_ms = now;
    }

    if ((raw_state != input->stable_state) &&
        interval_elapsed(now, input->last_change_ms, 30UL)) {
        input->stable_state = raw_state;
        return true;
    }

    return false;
}

static void servo_init(void)
{
    DDRB |= (1 << SERVO_BIT);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 39999U;
    OCR1A = 2000U;
}

static void dc_motor_set(uint8_t enabled)
{
    if (enabled) {
        PORTD |= (1 << DC_AIN1_BIT);
        PORTD &= ~(1 << DC_AIN2_BIT);
        PORTB |= (1 << DC_PWMA_BIT);
        g_dc_running = 1U;
    } else {
        PORTD &= ~((1 << DC_AIN1_BIT) | (1 << DC_AIN2_BIT));
        PORTB &= ~(1 << DC_PWMA_BIT);
        g_dc_running = 0U;
    }
}

static void dc_motor_pause(uint32_t now)
{
    dc_motor_set(0U);
    g_dc_resume_ms = now + DC_STOP_MS;
}

static void dc_motor_service(uint32_t now)
{
    if ((!g_dc_running) && time_reached(now, g_dc_resume_ms)) {
        dc_motor_set(1U);
    }
}

static void system_pause_start(uint32_t now)
{
    if (servo_state != SERVO_IDLE) {
        servo_deadline_ms += SYSTEM_PAUSE_MS;
    }

    if (g_dc_running || time_reached(now, g_dc_resume_ms)) {
        g_dc_resume_ms = now + SYSTEM_PAUSE_MS;
    } else {
        g_dc_resume_ms += SYSTEM_PAUSE_MS;
    }

    dc_motor_set(0U);
    g_system_paused = 1U;
    g_system_pause_until_ms = now + SYSTEM_PAUSE_MS;
}

static void system_pause_service(uint32_t now)
{
    uint8_t command = g_i2c_rx_command;

    if (command == SLAVE_CMD_PAUSE_5S) {
        g_i2c_rx_command = 0U;
        system_pause_start(now);
    }

    if (g_system_paused && time_reached(now, g_system_pause_until_ms)) {
        g_system_paused = 0U;
    }
}

static void servo_set_degrees(uint8_t degrees)
{
    uint16_t pulse_us;

    if (degrees > 180U) {
        degrees = 180U;
    }

    pulse_us = (uint16_t)(1000UL + ((uint32_t)degrees * 1000UL / 180UL));
    OCR1A = (uint16_t)(pulse_us * 2U);

    /*
     * Solo se cuenta una caja cuando el servo LLEGA a 90 grados
     * (y no estaba ya en 90). Antes se incrementaba en cualquier
     * cambio de angulo, lo que contaba 2 veces por cada caja real
     * (una vez al ir a 90 y otra vez al volver a 0).
     */
    if ((degrees == 90U) && (g_servo_position_deg != 90U)) {
        g_box_count++;
    }

    g_servo_position_deg = degrees;
}

static void servo_schedule(uint32_t now)
{
    if (servo_state == SERVO_IDLE) {
        servo_state = SERVO_WAITING;
        servo_deadline_ms = now + SERVO_WAIT_MS;
    }
}

static void servo_service(uint32_t now)
{
    if ((servo_state == SERVO_WAITING) && time_reached(now, servo_deadline_ms)) {
        dc_motor_pause(now);
        servo_set_degrees(90U);
        servo_state = SERVO_AT_90;
        servo_deadline_ms = now + SERVO_HOLD_MS;
    } else if ((servo_state == SERVO_AT_90) && time_reached(now, servo_deadline_ms)) {
        servo_set_degrees(0U);
        servo_state = SERVO_IDLE;
    }
}

static void twi_init(void)
{
    TWSR = I2C_PRESCALER_BITS;
    TWBR = I2C_BITRATE_VALUE;
    TWAR = (uint8_t)(I2C_SLAVE_ADDRESS << 1);
    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
}

static void i2c_update_status(void)
{
    g_i2c_box_count = g_box_count;
}

int main(void)
{
    debounce_t proximity;
    uint32_t now;

    cli();
    gpio_init();
    timer0_millis_init();
    servo_init();
    twi_init();
    sei();

    servo_set_degrees(0U);
    dc_motor_set(1U);
    now = millis_get();
    debounce_init(&proximity, proximity_active_raw(), now);

    for (;;) {
        now = millis_get();
        system_pause_service(now);

        if (g_system_paused) {
            i2c_update_status();
            continue;
        }

        if (debounce_update(&proximity, proximity_active_raw(), now) &&
            proximity.stable_state) {
            servo_schedule(now);
        }

        servo_service(now);
        dc_motor_service(now);
        i2c_update_status();
    }
}
