#ifndef FONDOS_H
#define FONDOS_H

#include <stdint.h>

#define FONDO_ANCHO 320U
#define FONDO_ALTO 240U
#define FONDO_PIXELES (FONDO_ANCHO * FONDO_ALTO)
#define TOTAL_FONDOS 12U

#define FONDO_01_ACTIVO 1
#define FONDO_02_ACTIVO 1
#define FONDO_03_ACTIVO 1
#define FONDO_04_ACTIVO 1
#define FONDO_05_ACTIVO 1
#define FONDO_06_ACTIVO 1
#define FONDO_07_ACTIVO 1
#define FONDO_08_ACTIVO 1
#define FONDO_09_ACTIVO 1
#define FONDO_10_ACTIVO 1
#define FONDO_11_ACTIVO 1
#define FONDO_12_ACTIVO 1

#if FONDO_01_ACTIVO
extern const uint16_t fondo_01[];
#endif
#if FONDO_02_ACTIVO
extern const uint16_t fondo_02[];
#endif
#if FONDO_03_ACTIVO
extern const uint16_t fondo_03[];
#endif
#if FONDO_04_ACTIVO
extern const uint16_t fondo_04[];
#endif
#if FONDO_05_ACTIVO
extern const uint16_t fondo_05[];
#endif
#if FONDO_06_ACTIVO
extern const uint16_t fondo_06[];
#endif
#if FONDO_07_ACTIVO
extern const uint16_t fondo_07[];
#endif
#if FONDO_08_ACTIVO
extern const uint16_t fondo_08[];
#endif
#if FONDO_09_ACTIVO
extern const uint16_t fondo_09[];
#endif
#if FONDO_10_ACTIVO
extern const uint16_t fondo_10[];
#endif
#if FONDO_11_ACTIVO
extern const uint16_t fondo_11[];
#endif
#if FONDO_12_ACTIVO
extern const uint16_t fondo_12[];
#endif

#endif
