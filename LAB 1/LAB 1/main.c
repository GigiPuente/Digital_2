/*
 * LAB 1.c
 *
 * Created: 7/9/2026
 * Author: Jorge Puente
 * Description:
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "display.h"

/****************************************/
// Variables globales y defines

//Entradas botones
#define BOTON_INICIO PC2
#define BOTON_J1 PC0
#define BOTON_J2 PC1

//Variables para empezar
volatile uint8_t jugadores_habilitados = 0;
volatile uint8_t conteo_activo = 0;
volatile uint8_t numero_conteo = 0;
volatile uint8_t tiempo_timer = 0;

//Guardar avances
static uint8_t avance_J1 = 0;
static uint8_t avance_J2 = 0;

/****************************************/
// Function prototypes

static void puertos(void);
static void inicializar_timer(void);
static uint8_t leer_boton(uint8_t pin_boton);
static void iniciar_conteo(void);
static void revisar_Js(void);

/****************************************/
// Main Function

int main(void)
{
    //Configuracion inicial
    puertos();
    disp_ini();
    display_OFF();
    inicializar_timer();

    //Activa interrupciones
    sei();

    while (1) {
        //Si se apacha empezar la cuenta
        if (leer_boton(1 << BOTON_INICIO) && !conteo_activo) {
            iniciar_conteo();
        }

        //Revisar si los jugadores pueden avanzar
        revisar_Js();
    }
}

/****************************************/
// NON-Interrupt subroutines

static void puertos(void)
{
    //Entradas (Botones)
    DDRC &= (uint8_t)~((1 << BOTON_INICIO) | (1 << BOTON_J1) | (1 << BOTON_J2));
    //Pull-up
    PORTC |= (1 << BOTON_INICIO) | (1 << BOTON_J1) | (1 << BOTON_J2);
}

static void inicializar_timer(void)
{
    //Timer1 CTC
    TCCR1A = 0;
    TCCR1B = (1 << WGM12);
    //Tiempo de 100 ms
    OCR1A = 6249;
    //Interrupcion timer
    TIMSK1 = (1 << OCIE1A);
    //Prescaler
    TCCR1B |= (1 << CS12);
}

static uint8_t leer_boton(uint8_t pin_boton)
{
    //Si el boton esta presionado
    if ((PINC & pin_boton) == 0) {
        //Antirrebote
        for (volatile uint16_t pausa = 0; pausa < 3000; pausa++) {
        }

        if ((PINC & pin_boton) == 0) {
            //Esperar a que se suelte el boton
            while ((PINC & pin_boton) == 0) {
            }
            return 1;
        }
    }

    return 0;
}

static void iniciar_conteo(void)
{
    cli();
    //Bloquear jugadores en el conteo
    jugadores_habilitados = 0;
    conteo_activo = 1;
    numero_conteo = 5;
    tiempo_timer = 0;
    sei();

    mostrar_numero(numero_conteo);
}

static void revisar_Js(void)
{
    //No avanzar si no ha terminado el conteo
    if (!jugadores_habilitados) {
        return;
    }

    //Sumar avance J1
    if (leer_boton(1 << BOTON_J1)) {
        avance_J1++;
    }

    //Sumar avance J2
    if (leer_boton(1 << BOTON_J2)) {
        avance_J2++;
    }
}

/****************************************/
// Interrupt routines

ISR(TIMER1_COMPA_vect)
{
    //Si no hay conteo, no hace nada
    if (!conteo_activo) {
        return;
    }

    //Contar las llamadas del timer
    tiempo_timer++;

    //Esperar a que pase 1 segundo
    if (tiempo_timer < 10) {
        return;
    }

    tiempo_timer = 0;

    //Bajar el numero del conteo
    if (numero_conteo > 0) {
        numero_conteo--;
        mostrar_numero(numero_conteo);
    }

    //Al 0, habilitar a todos
    if (numero_conteo == 0) {
        conteo_activo = 0;
        jugadores_habilitados = 1;
    }
}
