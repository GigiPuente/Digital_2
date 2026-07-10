#include "display.h"

#include <avr/io.h>

#define MASC_SEG 0b11111011

//Encender los numeros del display segun el binario
static void seg_ON(uint8_t numero_binario);

void disp_ini(void)
{
    //Salida
    DDRD |= 0b11111011;
    //Apagado
    PORTD &= (uint8_t)~0b11111011;
}

void mostrar_numero(uint8_t numero)
{
    //Valores binarios y encenderlos
    if (numero == 0) {
        seg_ON(0b01111011);
    } else if (numero == 1) {
        seg_ON(0b00001001);
    } else if (numero == 2) {
        seg_ON(0b10110011);
    } else if (numero == 3) {
        seg_ON(0b10011011);
    } else if (numero == 4) {
        seg_ON(0b11001001);
    } else if (numero == 5) {
        seg_ON(0b11011010);
    } else if (numero == 6) {
        seg_ON(0b11111010);
    } else if (numero == 7) {
        seg_ON(0b00001011);
    } else if (numero == 8) {
        seg_ON(0b11111011);
    } else if (numero == 9) {
        seg_ON(0b11011011);
    } else {
        display_OFF();
    }
}

void display_OFF(void)
{
    //Apagar todo
    PORTD &= (uint8_t)~0b11111011;
}

static void seg_ON(uint8_t numero_binario)
{
    //Actualiza solo los pines del display
    PORTD = (PORTD & (uint8_t)~MASC_SEG) | numero_binario;
}
