/*
 * main.c
 *
 * Esclavo I2C con todos los sensores y motores.
 * El maestro externo lee estados por A4 (SDA) y A5 (SCL).
 *
 * Registros I2C:
 * 0  motor DC ON/OFF
 * 1  proximidad activa
 * 2  estado del servo
 * 3  angulo del servo
 * 4  ultimo color detectado
 * 5  estado del stepper
 * 6  HX711 byte 3 (MSB)
 * 7  HX711 byte 2
 * 8  HX711 byte 1
 * 9  HX711 byte 0 (LSB)
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define PROX_BIT          PD2
#define BUTTON_BIT        PD3
#define HX_DOUT_BIT       PD4
#define HX_SCK_BIT        PD5
#define DC_MOTOR_BIT      PD6
#define STEPPER_STEP_BIT  PD7

#define STEPPER_DIR_BIT   PB0
#define SERVO_BIT         PB1
#define STEPPER_EN_BIT    PB2

#define PROX_ACTIVE_LOW             1
#define SERVO_WAIT_MS               10000UL
#define SERVO_HOLD_MS               1000UL
#define STEPPER_WAIT_MS             5000UL
#define STEPPER_HOLD_MS             1000UL
#define STEPPER_HALF_PERIOD_MS      2UL
#define STEPPER_FULL_STEPS_REV      200UL
#define STEPPER_MICROSTEPS          1UL
#define STEPPER_STEPS_90 \
    ((STEPPER_FULL_STEPS_REV * STEPPER_MICROSTEPS) / 4UL)

#define RED_DIR_LEVEL               1
#define STEPPER_DISABLE_AT_HOME     1

#define COLOR_SAMPLE_MS             150UL
#define COLOR_MIN_CLEAR             150U
#define COLOR_DOMINANCE_PERCENT     125UL
#define COLOR_REARM_SAMPLES         3U

#define WEIGHT_SAMPLE_MS            250UL
#define HX711_OFFSET                0L
#define HX711_COUNTS_PER_GRAM       0L

#define I2C_SLAVE_ADDRESS           0x12U

#define TWI_START_STATUS            0x08U
#define TWI_REP_START_STATUS        0x10U
#define TWI_MT_SLA_ACK_STATUS       0x18U
#define TWI_MT_DATA_ACK_STATUS      0x28U
#define TWI_MR_SLA_ACK_STATUS       0x40U
#define TWI_MR_DATA_ACK_STATUS      0x50U
#define TWI_MR_DATA_NACK_STATUS     0x58U

#define APDS9960_ADDR               0x39U
#define APDS_ENABLE                 0x80U
#define APDS_ATIME                  0x81U
#define APDS_ID                     0x92U
#define APDS_STATUS                 0x93U
#define APDS_CDATAL                 0x94U
#define APDS_CONTROL                0x8FU

static volatile uint32_t g_millis = 0UL;
static volatile uint8_t g_i2c_reg_pointer = 0U;
static volatile uint8_t g_i2c_regs[10] = {0U};

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

typedef enum {
    COLOR_NONE = 0,
    COLOR_RED,
    COLOR_BLUE
} detected_color_t;

typedef enum {
    STEPPER_IDLE = 0,
    STEPPER_WAITING,
    STEPPER_MOVING_OUT,
    STEPPER_HOLDING,
    STEPPER_MOVING_HOME
} stepper_state_t;

static servo_state_t servo_state = SERVO_IDLE;
static stepper_state_t stepper_state = STEPPER_IDLE;
static detected_color_t stepper_color = COLOR_NONE;
static detected_color_t last_color_detected = COLOR_NONE;

static uint32_t servo_deadline_ms = 0UL;
static uint32_t stepper_deadline_ms = 0UL;
static uint32_t stepper_next_toggle_ms = 0UL;
static uint32_t stepper_steps_done = 0UL;
static uint8_t stepper_step_high = 0U;
static uint8_t stepper_out_direction = 0U;
static uint8_t servo_angle_deg = 0U;
static int32_t last_weight_raw = 0L;

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
        g_i2c_reg_pointer = TWDR;
        TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
        break;

    case 0xA8U:
    case 0xB0U:
    case 0xB8U:
        TWDR = g_i2c_regs[g_i2c_reg_pointer % sizeof(g_i2c_regs)];
        g_i2c_reg_pointer++;
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

static void uart_init(void)
{
    const uint16_t ubrr = (uint16_t)((F_CPU / (16UL * 9600UL)) - 1UL);

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static void uart_putc(char c)
{
    while ((UCSR0A & (1 << UDRE0)) == 0U) {
    }
    UDR0 = (uint8_t)c;
}

static void uart_puts(const char *text)
{
    while (*text != '\0') {
        uart_putc(*text++);
    }
}

static void uart_put_u32(uint32_t value)
{
    char buffer[11];
    uint8_t index = 0U;

    if (value == 0UL) {
        uart_putc('0');
        return;
    }

    while ((value > 0UL) && (index < sizeof(buffer))) {
        buffer[index] = (char)('0' + (value % 10UL));
        value /= 10UL;
        index++;
    }

    while (index > 0U) {
        index--;
        uart_putc(buffer[index]);
    }
}

static void uart_put_i32(int32_t value)
{
    uint32_t magnitude;

    if (value < 0L) {
        uart_putc('-');
        magnitude = (uint32_t)(-(value + 1L));
        magnitude += 1UL;
    } else {
        magnitude = (uint32_t)value;
    }

    uart_put_u32(magnitude);
}

static void uart_put_u16(uint16_t value)
{
    uart_put_u32((uint32_t)value);
}

static uint8_t button_pressed_raw(void)
{
    return ((PIND & (1 << BUTTON_BIT)) == 0U);
}

static uint8_t proximity_active_raw(void)
{
#if PROX_ACTIVE_LOW
    return ((PIND & (1 << PROX_BIT)) == 0U);
#else
    return ((PIND & (1 << PROX_BIT)) != 0U);
#endif
}

static void dc_motor_set(uint8_t enabled)
{
    if (enabled) {
        PORTD |= (1 << DC_MOTOR_BIT);
    } else {
        PORTD &= ~(1 << DC_MOTOR_BIT);
    }
}

static void gpio_init(void)
{
    DDRD &= ~((1 << PROX_BIT) | (1 << BUTTON_BIT) | (1 << HX_DOUT_BIT));
    PORTD |= (1 << BUTTON_BIT) | (1 << PROX_BIT);

    DDRD |= (1 << HX_SCK_BIT) | (1 << DC_MOTOR_BIT) | (1 << STEPPER_STEP_BIT);
    PORTD &= ~((1 << HX_SCK_BIT) | (1 << DC_MOTOR_BIT) | (1 << STEPPER_STEP_BIT));

    DDRB |= (1 << STEPPER_DIR_BIT) | (1 << STEPPER_EN_BIT);
    PORTB &= ~(1 << STEPPER_DIR_BIT);
    PORTB |= (1 << STEPPER_EN_BIT);
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

static void servo_set_degrees(uint8_t degrees)
{
    uint16_t pulse_us;

    if (degrees > 180U) {
        degrees = 180U;
    }

    pulse_us = (uint16_t)(1000UL + ((uint32_t)degrees * 1000UL / 180UL));
    OCR1A = (uint16_t)(pulse_us * 2U);
    servo_angle_deg = degrees;
}

static void servo_schedule(uint32_t now)
{
    if (servo_state == SERVO_IDLE) {
        servo_state = SERVO_WAITING;
        servo_deadline_ms = now + SERVO_WAIT_MS;
        uart_puts("Proximidad detectada: servo programado.\r\n");
    }
}

static void servo_service(uint32_t now)
{
    if ((servo_state == SERVO_WAITING) && time_reached(now, servo_deadline_ms)) {
        servo_set_degrees(90U);
        servo_state = SERVO_AT_90;
        servo_deadline_ms = now + SERVO_HOLD_MS;
        uart_puts("Servo a 90 grados.\r\n");
    } else if ((servo_state == SERVO_AT_90) && time_reached(now, servo_deadline_ms)) {
        servo_set_degrees(0U);
        servo_state = SERVO_IDLE;
        uart_puts("Servo regreso a 0 grados.\r\n");
    }
}

static void twi_init(void)
{
    TWSR = 0x00U;
    TWBR = (uint8_t)(((F_CPU / 100000UL) - 16UL) / 2UL);
    TWAR = (uint8_t)(I2C_SLAVE_ADDRESS << 1);
    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
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
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN) |
           (1 << TWEA) | (1 << TWIE);
}

static bool apds_write_register(uint8_t reg, uint8_t value)
{
    if (!twi_start((uint8_t)(APDS9960_ADDR << 1))) {
        return false;
    }

    if (!twi_write(reg) || !twi_write(value)) {
        twi_stop();
        return false;
    }

    twi_stop();
    return true;
}

static bool apds_read_bytes(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t i;

    if ((data == NULL) || (length == 0U)) {
        return false;
    }

    if (!twi_start((uint8_t)(APDS9960_ADDR << 1))) {
        return false;
    }

    if (!twi_write(reg)) {
        twi_stop();
        return false;
    }

    if (!twi_start((uint8_t)((APDS9960_ADDR << 1) | 1U))) {
        twi_stop();
        return false;
    }

    for (i = 0U; i < length; i++) {
        if (!twi_read(&data[i], i < (uint8_t)(length - 1U))) {
            twi_stop();
            return false;
        }
    }

    twi_stop();
    return true;
}

static bool apds_read_register(uint8_t reg, uint8_t *value)
{
    return apds_read_bytes(reg, value, 1U);
}

static bool apds9960_init(void)
{
    uint8_t id;

    if (!apds_read_register(APDS_ID, &id)) {
        return false;
    }

    uart_puts("APDS-9960 ID: ");
    uart_put_u16(id);
    uart_puts("\r\n");

    if (!apds_write_register(APDS_ENABLE, 0x00U)) {
        return false;
    }

    if (!apds_write_register(APDS_ATIME, 0xDBU)) {
        return false;
    }

    if (!apds_write_register(APDS_CONTROL, 0x01U)) {
        return false;
    }

    if (!apds_write_register(APDS_ENABLE, 0x03U)) {
        return false;
    }

    _delay_ms(120);
    return true;
}

static bool apds9960_read_rgbc(uint16_t *clear,
                               uint16_t *red,
                               uint16_t *green,
                               uint16_t *blue)
{
    uint8_t status;
    uint8_t data[8];

    if (!apds_read_register(APDS_STATUS, &status)) {
        return false;
    }

    if ((status & 0x01U) == 0U) {
        return false;
    }

    if (!apds_read_bytes(APDS_CDATAL, data, sizeof(data))) {
        return false;
    }

    *clear = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    *red   = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
    *green = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
    *blue  = (uint16_t)data[6] | ((uint16_t)data[7] << 8);

    return true;
}

static detected_color_t classify_color(uint16_t clear,
                                       uint16_t red,
                                       uint16_t green,
                                       uint16_t blue)
{
    if (clear < COLOR_MIN_CLEAR) {
        return COLOR_NONE;
    }

    if (((uint32_t)red * 100UL > (uint32_t)blue * COLOR_DOMINANCE_PERCENT) &&
        (red > green)) {
        return COLOR_RED;
    }

    if (((uint32_t)blue * 100UL > (uint32_t)red * COLOR_DOMINANCE_PERCENT) &&
        (blue > green)) {
        return COLOR_BLUE;
    }

    return COLOR_NONE;
}

static void stepper_set_direction(uint8_t level)
{
    if (level) {
        PORTB |= (1 << STEPPER_DIR_BIT);
    } else {
        PORTB &= ~(1 << STEPPER_DIR_BIT);
    }
}

static void stepper_enable(uint8_t enabled)
{
    if (enabled) {
        PORTB &= ~(1 << STEPPER_EN_BIT);
    } else {
        PORTB |= (1 << STEPPER_EN_BIT);
    }
}

static void stepper_schedule(detected_color_t color, uint32_t now)
{
    if ((stepper_state != STEPPER_IDLE) ||
        ((color != COLOR_RED) && (color != COLOR_BLUE))) {
        return;
    }

    stepper_color = color;
    stepper_state = STEPPER_WAITING;
    stepper_deadline_ms = now + STEPPER_WAIT_MS;

    if (color == COLOR_RED) {
        uart_puts("Rojo detectado: stepper programado.\r\n");
    } else {
        uart_puts("Azul detectado: stepper programado.\r\n");
    }
}

static void stepper_begin_move(stepper_state_t new_state,
                               uint8_t direction,
                               uint32_t now)
{
    stepper_out_direction = direction;
    stepper_set_direction(direction);
    stepper_enable(1U);

    PORTD &= ~(1 << STEPPER_STEP_BIT);
    stepper_step_high = 0U;
    stepper_steps_done = 0UL;
    stepper_next_toggle_ms = now + 1UL;
    stepper_state = new_state;
}

static void stepper_finish_pulse_train(uint32_t now)
{
    PORTD &= ~(1 << STEPPER_STEP_BIT);
    stepper_step_high = 0U;

    if (stepper_state == STEPPER_MOVING_OUT) {
        stepper_state = STEPPER_HOLDING;
        stepper_deadline_ms = now + STEPPER_HOLD_MS;
        uart_puts("Stepper llego a 90 grados.\r\n");
    } else {
#if STEPPER_DISABLE_AT_HOME
        stepper_enable(0U);
#endif
        stepper_state = STEPPER_IDLE;
        stepper_color = COLOR_NONE;
        uart_puts("Stepper regreso al origen.\r\n");
    }
}

static void stepper_generate_pulses(uint32_t now)
{
    if (!time_reached(now, stepper_next_toggle_ms)) {
        return;
    }

    stepper_next_toggle_ms += STEPPER_HALF_PERIOD_MS;

    if (stepper_step_high) {
        PORTD &= ~(1 << STEPPER_STEP_BIT);
        stepper_step_high = 0U;

        if (stepper_steps_done >= STEPPER_STEPS_90) {
            stepper_finish_pulse_train(now);
        }
    } else {
        PORTD |= (1 << STEPPER_STEP_BIT);
        stepper_step_high = 1U;
        stepper_steps_done++;
    }
}

static void stepper_service(uint32_t now)
{
    switch (stepper_state) {
    case STEPPER_WAITING:
        if (time_reached(now, stepper_deadline_ms)) {
            const uint8_t direction =
                (stepper_color == COLOR_RED) ? RED_DIR_LEVEL : (uint8_t)!RED_DIR_LEVEL;
            stepper_begin_move(STEPPER_MOVING_OUT, direction, now);
            uart_puts("Stepper inicia movimiento de salida.\r\n");
        }
        break;

    case STEPPER_MOVING_OUT:
    case STEPPER_MOVING_HOME:
        stepper_generate_pulses(now);
        break;

    case STEPPER_HOLDING:
        if (time_reached(now, stepper_deadline_ms)) {
            stepper_begin_move(STEPPER_MOVING_HOME,
                               (uint8_t)!stepper_out_direction,
                               now);
            uart_puts("Stepper inicia retorno.\r\n");
        }
        break;

    default:
        break;
    }
}

static bool hx711_ready(void)
{
    return ((PIND & (1 << HX_DOUT_BIT)) == 0U);
}

static int32_t hx711_read_raw(void)
{
    uint32_t value = 0UL;
    uint8_t i;
    uint8_t old_sreg;

    if (!hx711_ready()) {
        return last_weight_raw;
    }

    old_sreg = SREG;
    cli();

    for (i = 0U; i < 24U; i++) {
        PORTD |= (1 << HX_SCK_BIT);
        _delay_us(1);

        value <<= 1;
        if ((PIND & (1 << HX_DOUT_BIT)) != 0U) {
            value++;
        }

        PORTD &= ~(1 << HX_SCK_BIT);
        _delay_us(1);
    }

    PORTD |= (1 << HX_SCK_BIT);
    _delay_us(1);
    PORTD &= ~(1 << HX_SCK_BIT);
    _delay_us(1);

    SREG = old_sreg;

    if ((value & 0x00800000UL) != 0UL) {
        value |= 0xFF000000UL;
    }

    return (int32_t)value;
}

static void weight_service(uint32_t now)
{
    static uint32_t last_read_ms = 0UL;

    if (!interval_elapsed(now, last_read_ms, WEIGHT_SAMPLE_MS)) {
        return;
    }

    last_read_ms = now;

    if (hx711_ready()) {
        last_weight_raw = hx711_read_raw();

        uart_puts("HX711 bruto: ");
        uart_put_i32(last_weight_raw);

#if HX711_COUNTS_PER_GRAM != 0
        uart_puts(" | peso aproximado: ");
        uart_put_i32((last_weight_raw - HX711_OFFSET) / HX711_COUNTS_PER_GRAM);
        uart_puts(" g");
#endif
        uart_puts("\r\n");
    }
}

static void color_service(uint32_t now)
{
    static uint32_t last_sample_ms = 0UL;
    static uint8_t color_armed = 1U;
    static uint8_t no_color_samples = 0U;
    uint16_t clear;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    detected_color_t color;

    if (!interval_elapsed(now, last_sample_ms, COLOR_SAMPLE_MS)) {
        return;
    }

    last_sample_ms = now;

    if (!apds9960_read_rgbc(&clear, &red, &green, &blue)) {
        return;
    }

    color = classify_color(clear, red, green, blue);

    if (color == COLOR_NONE) {
        if (no_color_samples < COLOR_REARM_SAMPLES) {
            no_color_samples++;
        }

        if (no_color_samples >= COLOR_REARM_SAMPLES) {
            color_armed = 1U;
        }
        return;
    }

    no_color_samples = 0U;

    if (color_armed && (stepper_state == STEPPER_IDLE)) {
        color_armed = 0U;
        last_color_detected = color;

        uart_puts("Color C=");
        uart_put_u16(clear);
        uart_puts(" R=");
        uart_put_u16(red);
        uart_puts(" G=");
        uart_put_u16(green);
        uart_puts(" B=");
        uart_put_u16(blue);
        uart_puts("\r\n");

        stepper_schedule(color, now);
    }
}

static void i2c_update_status(uint8_t dc_motor_on)
{
    uint32_t raw = (uint32_t)last_weight_raw;

    g_i2c_regs[0] = dc_motor_on;
    g_i2c_regs[1] = proximity_active_raw();
    g_i2c_regs[2] = (uint8_t)servo_state;
    g_i2c_regs[3] = servo_angle_deg;
    g_i2c_regs[4] = (uint8_t)last_color_detected;
    g_i2c_regs[5] = (uint8_t)stepper_state;
    g_i2c_regs[6] = (uint8_t)(raw >> 24);
    g_i2c_regs[7] = (uint8_t)(raw >> 16);
    g_i2c_regs[8] = (uint8_t)(raw >> 8);
    g_i2c_regs[9] = (uint8_t)(raw);
}

int main(void)
{
    debounce_t button;
    debounce_t proximity;
    uint8_t dc_motor_on = 0U;
    uint32_t now;

    cli();
    gpio_init();
    uart_init();
    timer0_millis_init();
    servo_init();
    twi_init();
    sei();

    _delay_ms(100);
    uart_puts("\r\nSistema esclavo iniciado.\r\n");

    if (apds9960_init()) {
        uart_puts("APDS-9960 iniciado correctamente.\r\n");
    } else {
        uart_puts("ERROR: no se pudo iniciar el APDS-9960.\r\n");
    }

    servo_set_degrees(0U);
    now = millis_get();
    debounce_init(&button, button_pressed_raw(), now);
    debounce_init(&proximity, proximity_active_raw(), now);

    for (;;) {
        now = millis_get();

        if (debounce_update(&button, button_pressed_raw(), now) &&
            button.stable_state) {
            dc_motor_on = (uint8_t)!dc_motor_on;
            dc_motor_set(dc_motor_on);

            if (dc_motor_on) {
                uart_puts("Motor DC encendido.\r\n");
            } else {
                uart_puts("Motor DC apagado.\r\n");
            }
        }

        if (debounce_update(&proximity, proximity_active_raw(), now) &&
            proximity.stable_state) {
            servo_schedule(now);
        }

        servo_service(now);
        color_service(now);
        stepper_service(now);
        weight_service(now);
        i2c_update_status(dc_motor_on);
    }
}
