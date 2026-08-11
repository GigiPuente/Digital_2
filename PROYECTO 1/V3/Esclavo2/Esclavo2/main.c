/*
 * Esclavo2.c
 *
 * Created:
 * Author:
 * Description:
 */
/****************************************/
// Encabezado (Libraries)

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

// Pines del stepper
#define STEPPER_IN1_BIT             PD4
#define STEPPER_IN2_BIT             PD5
#define STEPPER_IN3_BIT             PD6
#define STEPPER_IN4_BIT             PD7

// Pines del sensor ultrasonico y boton
#define ULTRASONIC_TRIG_BIT         PB0
#define ULTRASONIC_ECHO_BIT         PB1
#define BUTTON_BIT                  PB2

// Pines I2C
#define TWI_SDA_BIT                 PC4
#define TWI_SCL_BIT                 PC5

// Direccion y velocidad I2C
#define I2C_SLAVE_ADDRESS           0x13U
#define I2C_PRESCALER_BITS          0x03U
#define I2C_BITRATE_VALUE           12U

// Configuracion del sensor ultrasonico
#define ULTRASONIC_SAMPLE_MS        200UL
#define ULTRASONIC_MAX_ECHO_US      30000UL
#define ULTRASONIC_TRIGGER_CM       10L
#define ULTRASONIC_REARM_CM         15L
#define ULTRASONIC_DETECT_GRACE_MS  500UL
#define ULTRASONIC_STOP_HOLD_MS     3000UL

// Configuracion del boton y stepper
#define BUTTON_DEBOUNCE_MS          30UL
#define STEPPER_STEP_DELAY_MS       3UL
#define STEPPER_STEPS_180           1024U
#define STEPPER_HOLD_MS             3000UL

// Variables globales del esclavo 2
static volatile uint32_t g_millis = 0UL;
static volatile uint8_t g_i2c_regs[8] = {0U};
static volatile uint8_t g_i2c_reg_pointer = 0U;

typedef struct {
    uint8_t stable_state;
    uint8_t last_raw_state;
    uint32_t last_change_ms;
} debounce_t;

typedef enum {
    STEPPER_IDLE = 0,
    STEPPER_MOVING_OUT,
    STEPPER_HOLDING,
    STEPPER_MOVING_HOME
} stepper_state_t;

static stepper_state_t g_stepper_state = STEPPER_IDLE;
static uint8_t g_stepper_phase = 0U;
static uint16_t g_stepper_steps_remaining = 0U;
static uint32_t g_stepper_next_step_ms = 0UL;
static uint32_t g_stepper_hold_until_ms = 0UL;
static uint8_t g_cycle_count = 0U;
static int32_t g_last_distance_cm = -1L;
static uint8_t g_button_state = 0U;
static uint8_t g_stop_active = 0U;
static uint32_t g_detect_elapsed_ms = 0UL;

/****************************************/
// Function prototypes

// Tiempo y utilidades
static uint32_t millis_get(void);
static bool time_reached(uint32_t now, uint32_t deadline);
static bool interval_elapsed(uint32_t now, uint32_t previous, uint32_t interval);
static void timer0_millis_init(void);

// Configuracion de pines
static void gpio_init(void);

// Boton
static void debounce_init(debounce_t *input, uint8_t initial_state, uint32_t now);
static bool debounce_update(debounce_t *input, uint8_t raw_state, uint32_t now);
static uint8_t button_pressed_raw(void);

// Stepper
static void stepper_outputs_off(void);
static void stepper_apply_phase(uint8_t phase);
static void stepper_start_cycle(uint32_t now);
static void stepper_service(uint32_t now);
static void stepper_force_stop(void);

// Sensor ultrasonico
static bool ultrasonic_read_cm(int32_t *distance_cm);
static void ultrasonic_service(uint32_t now);

// I2C
static void twi_init(void);
static void i2c_update_status(void);

/****************************************/
// Main Function

int main(void)
{
    debounce_t button;
    uint32_t now;

    cli();
    gpio_init();
    timer0_millis_init();
    twi_init();
    sei();

    now = millis_get();
    debounce_init(&button, button_pressed_raw(), now);

    for (;;) {
        now = millis_get();

        // Lectura del boton
        g_button_state = button_pressed_raw();
        if (!g_stop_active &&
            debounce_update(&button, button_pressed_raw(), now) &&
            button.stable_state) {
            stepper_start_cycle(now);
        }

        // Lectura de distancia y control de paro
        ultrasonic_service(now);
        if (!g_stop_active) {
            stepper_service(now);
        }

        // Actualizacion I2C
        i2c_update_status();
    }
}

/****************************************/
// NON-Interrupt subroutines

// Tiempo y utilidades
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

// Configuracion de pines
static void gpio_init(void)
{
    DDRD |= (1 << STEPPER_IN1_BIT) | (1 << STEPPER_IN2_BIT) |
            (1 << STEPPER_IN3_BIT) | (1 << STEPPER_IN4_BIT);
    PORTD &= ~((1 << STEPPER_IN1_BIT) | (1 << STEPPER_IN2_BIT) |
               (1 << STEPPER_IN3_BIT) | (1 << STEPPER_IN4_BIT));

    DDRB |= (1 << ULTRASONIC_TRIG_BIT);
    DDRB &= ~((1 << ULTRASONIC_ECHO_BIT) | (1 << BUTTON_BIT));
    PORTB &= ~(1 << ULTRASONIC_TRIG_BIT);
    PORTB |= (1 << BUTTON_BIT);

    DDRC &= ~((1 << TWI_SDA_BIT) | (1 << TWI_SCL_BIT));
    PORTC |= (1 << TWI_SDA_BIT) | (1 << TWI_SCL_BIT);
}

// Boton
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
        interval_elapsed(now, input->last_change_ms, BUTTON_DEBOUNCE_MS)) {
        input->stable_state = raw_state;
        return true;
    }

    return false;
}

static uint8_t button_pressed_raw(void)
{
    return ((PINB & (1 << BUTTON_BIT)) == 0U);
}

// Stepper
static void stepper_outputs_off(void)
{
    PORTD &= ~((1 << STEPPER_IN1_BIT) | (1 << STEPPER_IN2_BIT) |
               (1 << STEPPER_IN3_BIT) | (1 << STEPPER_IN4_BIT));
}

static void stepper_apply_phase(uint8_t phase)
{
    stepper_outputs_off();

    switch (phase & 0x03U) {
    case 0U:
        PORTD |= (1 << STEPPER_IN1_BIT);
        break;
    case 1U:
        PORTD |= (1 << STEPPER_IN2_BIT);
        break;
    case 2U:
        PORTD |= (1 << STEPPER_IN3_BIT);
        break;
    default:
        PORTD |= (1 << STEPPER_IN4_BIT);
        break;
    }
}

static void stepper_start_cycle(uint32_t now)
{
    if (g_stepper_state != STEPPER_IDLE) {
        return;
    }

    g_stepper_state = STEPPER_MOVING_OUT;
    g_stepper_steps_remaining = STEPPER_STEPS_180;
    g_stepper_next_step_ms = now;
    g_cycle_count++;
}

static void stepper_service(uint32_t now)
{
    if (g_stepper_state == STEPPER_IDLE) {
        return;
    }

    if (g_stepper_state == STEPPER_HOLDING) {
        if (time_reached(now, g_stepper_hold_until_ms)) {
            g_stepper_state = STEPPER_MOVING_HOME;
            g_stepper_steps_remaining = STEPPER_STEPS_180;
            g_stepper_next_step_ms = now;
        }
        return;
    }

    if (!time_reached(now, g_stepper_next_step_ms)) {
        return;
    }

    g_stepper_next_step_ms = now + STEPPER_STEP_DELAY_MS;

    if (g_stepper_state == STEPPER_MOVING_OUT) {
        g_stepper_phase = (uint8_t)((g_stepper_phase + 1U) & 0x03U);
    } else {
        g_stepper_phase = (uint8_t)((g_stepper_phase + 3U) & 0x03U);
    }

    stepper_apply_phase(g_stepper_phase);

    if (g_stepper_steps_remaining > 0U) {
        g_stepper_steps_remaining--;
    }

    if (g_stepper_steps_remaining == 0U) {
        if (g_stepper_state == STEPPER_MOVING_OUT) {
            g_stepper_state = STEPPER_HOLDING;
            g_stepper_hold_until_ms = now + STEPPER_HOLD_MS;
        } else {
            g_stepper_state = STEPPER_IDLE;
            stepper_outputs_off();
        }
    }
}

static void stepper_force_stop(void)
{
    g_stepper_state = STEPPER_IDLE;
    g_stepper_steps_remaining = 0U;
    stepper_outputs_off();
}

// Sensor ultrasonico
static bool ultrasonic_read_cm(int32_t *distance_cm)
{
    uint32_t timeout_us;
    uint32_t pulse_us = 0UL;

    PORTB &= ~(1 << ULTRASONIC_TRIG_BIT);
    _delay_us(2);
    PORTB |= (1 << ULTRASONIC_TRIG_BIT);
    _delay_us(10);
    PORTB &= ~(1 << ULTRASONIC_TRIG_BIT);

    timeout_us = ULTRASONIC_MAX_ECHO_US;
    while (((PINB & (1 << ULTRASONIC_ECHO_BIT)) == 0U) && (timeout_us > 0UL)) {
        _delay_us(1);
        timeout_us--;
    }

    if (timeout_us == 0UL) {
        return false;
    }

    while (((PINB & (1 << ULTRASONIC_ECHO_BIT)) != 0U) &&
           (pulse_us < ULTRASONIC_MAX_ECHO_US)) {
        _delay_us(1);
        pulse_us++;
    }

    if ((pulse_us == 0UL) || (pulse_us >= ULTRASONIC_MAX_ECHO_US)) {
        return false;
    }

    *distance_cm = (int32_t)((pulse_us + 29UL) / 58UL);
    return true;
}

static void ultrasonic_service(uint32_t now)
{
    static uint32_t last_sample_ms = 0UL;
    static uint32_t detect_start_ms = 0UL;
    static uint32_t last_detect_seen_ms = 0UL;
    static uint8_t detect_active = 0U;
    int32_t distance_cm;
    uint8_t distance_valid;
    uint8_t detected_now;

    if (!interval_elapsed(now, last_sample_ms, ULTRASONIC_SAMPLE_MS)) {
        return;
    }

    last_sample_ms = now;

    distance_valid = ultrasonic_read_cm(&distance_cm) ? 1U : 0U;
    if (distance_valid) {
        g_last_distance_cm = distance_cm;
    } else {
        g_last_distance_cm = -1L;
        distance_cm = -1L;
    }

    detected_now = (uint8_t)(distance_valid &&
                             (distance_cm > 0L) &&
                             (distance_cm <= ULTRASONIC_TRIGGER_CM));

    if (detected_now) {
        if (!detect_active) {
            detect_active = 1U;
            detect_start_ms = now;
        }

        last_detect_seen_ms = now;
        g_detect_elapsed_ms = now - detect_start_ms;

        if (!g_stop_active &&
            interval_elapsed(now, detect_start_ms, ULTRASONIC_STOP_HOLD_MS)) {
            g_stop_active = 1U;
            stepper_force_stop();
        }
    } else {
        if (detect_active &&
            !interval_elapsed(now, last_detect_seen_ms, ULTRASONIC_DETECT_GRACE_MS)) {
            g_detect_elapsed_ms = now - detect_start_ms;
        } else {
            detect_active = 0U;
            g_detect_elapsed_ms = 0UL;
        }

        if (distance_valid && (distance_cm >= ULTRASONIC_REARM_CM)) {
            g_stop_active = 0U;
        }
    }

}

// I2C
static void twi_init(void)
{
    TWSR = I2C_PRESCALER_BITS;
    TWBR = I2C_BITRATE_VALUE;
    TWAR = (uint8_t)(I2C_SLAVE_ADDRESS << 1);
    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
}

static void i2c_update_status(void)
{
    uint32_t raw_detect_ms = g_detect_elapsed_ms;

    g_i2c_regs[0] = g_cycle_count;
    g_i2c_regs[1] = (uint8_t)g_stepper_state;
    g_i2c_regs[2] = (uint8_t)(raw_detect_ms >> 24);
    g_i2c_regs[3] = (uint8_t)(raw_detect_ms >> 16);
    g_i2c_regs[4] = (uint8_t)(raw_detect_ms >> 8);
    g_i2c_regs[5] = (uint8_t)raw_detect_ms;
    g_i2c_regs[6] = g_button_state;
    g_i2c_regs[7] = g_stop_active;
}

/****************************************/
// Interrupt routines

// Interrupcion del temporizador
ISR(TIMER0_COMPA_vect)
{
    g_millis++;
}

// Interrupcion I2C
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
        g_i2c_reg_pointer = TWDR;
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0xA0U:
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0xA8U:
    case 0xB0U:
    case 0xB8U:
    case 0xC8U:
        TWDR = g_i2c_regs[g_i2c_reg_pointer % sizeof(g_i2c_regs)];
        g_i2c_reg_pointer++;
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    default:
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;
    }
}
