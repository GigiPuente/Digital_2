/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  int16_t x;
  int16_t vx;
  int32_t y_fp;
  int32_t vy_fp;
  bool on_ground;
} Jugador_t;

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t ancho;
  int16_t alto;
} Plataforma_t;

typedef struct
{
  int16_t x;
  int16_t y;
  int8_t dx;
  int8_t dy;
  bool activo;
} DisparoBoss_t;

typedef enum
{
  ESTADO_MENU = 0,
  ESTADO_LOAD,
  ESTADO_JUEGO,
  ESTADO_PAUSA,
  ESTADO_DORMIDO
} Estado_t;

typedef struct
{
  uint16_t frecuencia;
  uint16_t duracion;
} Nota_t;

typedef enum
{
  AUDIO_SILENCIO = 0,
  AUDIO_MENU,
  AUDIO_PAUSA,
  AUDIO_ATERRIZAJE
} AudioEstado_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_ANCHO               320
#define LCD_ALTO                240
#define TOTAL_FONDOS             12U
#define FONDO_TAMANO_BYTES   153600UL

#define COLOR_NEGRO             0x0000U
#define COLOR_BLANCO            0xFFFFU
#define COLOR_CIELO             0x10A6U
#define COLOR_PISO              0x5AEBU
#define COLOR_PISO_TOP          0xC618U
#define COLOR_JUGADOR           0xFD20U
#define COLOR_JUGADOR_DARK      0xA260U
#define COLOR_MENU              0x0821U
#define COLOR_OPCION            0x18C3U
#define COLOR_SELECCION         0xFD20U
#define COLOR_TITULO            0xFFE0U
#define COLOR_BOSS_OSCURO       0x780FU
#define COLOR_BOSS_CLARO        0xF81FU
#define COLOR_BOSS_OJOS         0xFFE0U
#define COLOR_TRANSPARENTE      0x0000U

#define JUGADOR_ANCHO            16
#define JUGADOR_ALTO             20
#define WALK_FRAMES               2U
#define PLAYER_SPEED             3
#define WALK_FRAME_MS            120U
#define PISO_Y                   210
#define TOTAL_PANTALLAS           16U
#define PLATAFORMAS_PANTALLA       5U
#define BOSS_ANCHO                32
#define BOSS_ALTO                 36
#define BOSS_Y                    45
#define BOSS_SPEED                 1
#define BOSS_DISPAROS_MAX          8U
#define BOSS_DISPARO_ANCHO         5
#define BOSS_DISPARO_ALTO          8
#define BOSS_DISPARO_MS           320U
#define JUGADOR_VIDA_MAX            3U
#define DISPARO_ANCHO              3
#define DISPARO_ALTO               8
#define DISPARO_SPEED              6
#define PISTOLA_X                 245
#define PISTOLA_Y                 198
#define PISTOLA_ANCHO              18
#define PISTOLA_ALTO                9
#define FP_SHIFT                  8
#define FP_UNO                    (1L << FP_SHIFT)
#define GRAVEDAD_FP               50L
#define JUMP_MIN_SPEED_FP         (-512L)
#define JUMP_MAX_SPEED_FP         (-1792L)
#define JUMP_MIN_SPEED_X             1
#define JUMP_MAX_SPEED_X             3
#define JUMP_CHARGE_MS             700U
#define MAX_FALL_SPEED_FP         2048L
#define FRAME_TIME_MS             16U

#define CONTROL_VALIDO            0x80U
#define CONTROL_IZQUIERDA         0x01U
#define CONTROL_DERECHA           0x02U
#define CONTROL_SALTO             0x04U
#define CONTROL_DISPARO           0x08U
#define CONTROL_PAUSA             0x10U
#define CONTROL_TIMEOUT_MS         250U

#define MENU_EMPEZAR               0U
#define MENU_LOAD                  1U
#define MENU_SALIR                 2U
#define MENU_TOTAL                 3U
#define TOTAL_BOSSES               4U
#define PAUSA_CONTINUAR             0U
#define PAUSA_SALIR                 1U

#define AUDIO_TIMER_HZ        1000000UL

#define LCD_DATA_A_MASK           (LCD_D0_Pin | LCD_D2_Pin | LCD_D7_Pin)
#define LCD_DATA_B_MASK           (LCD_D3_Pin | LCD_D4_Pin | LCD_D5_Pin | LCD_D6_Pin)
#define LCD_DATA_C_MASK           (LCD_D1_Pin)

#define LCD_CS_LOW()              (LCD_CS_GPIO_Port->BSRR = ((uint32_t)LCD_CS_Pin << 16U))
#define LCD_CS_HIGH()             (LCD_CS_GPIO_Port->BSRR = LCD_CS_Pin)
#define LCD_RS_COMMAND()          (LCD_RS_GPIO_Port->BSRR = ((uint32_t)LCD_RS_Pin << 16U))
#define LCD_RS_DATA()             (LCD_RS_GPIO_Port->BSRR = LCD_RS_Pin)
#define LCD_WR_LOW()              (LCD_WR_GPIO_Port->BSRR = ((uint32_t)LCD_WR_Pin << 16U))
#define LCD_WR_HIGH()             (LCD_WR_GPIO_Port->BSRR = LCD_WR_Pin)
#define LCD_RD_LOW()              (LCD_RD_GPIO_Port->BSRR = ((uint32_t)LCD_RD_Pin << 16U))
#define LCD_RD_HIGH()             (LCD_RD_GPIO_Port->BSRR = LCD_RD_Pin)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim10;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static Jugador_t jugador;
static bool jump_apachado = false;
static bool salto_cargando = false;
static uint32_t salto_inicio = 0U;
static int8_t salto_direccion = 0;
static Estado_t estado_actual = ESTADO_MENU;
static uint8_t opcion_menu = MENU_EMPEZAR;
static uint8_t opcion_load = 0U;
static bool left_anterior = false;
static bool right_anterior = false;
static bool jump_anterior = false;
static bool shoot_anterior = false;
static bool pause_anterior = false;
static bool despertar_armado = false;
static uint8_t opcion_pausa = PAUSA_CONTINUAR;
static uint32_t pausa_inicio = 0U;
static uint8_t jugador_frame = 0U;
static bool jugador_mira_derecha = true;
static uint32_t proximo_frame_caminata = 0U;
static uint8_t pantalla_actual = 0U;
static bool bosses_derrotados[4] = {false, false, false, false};
static int16_t boss_x = 0;
static int8_t boss_direccion = 1;
static uint8_t boss_vida = 0U;
static bool disparo_activo = false;
static int16_t disparo_x = 0;
static int16_t disparo_y = 0;
static int16_t disparo_dx = 0;
static int16_t disparo_dy = 0;
static bool tiene_pistola = false;
static DisparoBoss_t disparos_boss[BOSS_DISPAROS_MAX];
static uint32_t proximo_disparo_boss = 0U;
static uint8_t patron_disparo_boss = 0U;
static uint8_t vida_jugador = 0U;
static bool salto_en_curso = false;
static uint8_t control_rx_byte = 0U;
static volatile uint8_t control_estado = 0U;
static volatile uint32_t control_ultimo_ms = 0U;
static FATFS sd_fatfs;
static FIL fondo_archivo;
static bool sd_lista = false;
static bool fondo_abierto = false;
static uint8_t fondo_indice_abierto = 0xFFU;
static uint16_t fondo_buffer[LCD_ANCHO];
static AudioEstado_t audio_estado = AUDIO_SILENCIO;
static uint8_t audio_indice = 0U;
static uint32_t audio_siguiente = 0U;
static const Nota_t musica_menu[] = {
  {523U, 140U}, {659U, 140U}, {784U, 180U}, {1047U, 260U},
  {0U, 70U}, {988U, 140U}, {784U, 140U}, {659U, 220U},
  {0U, 90U}, {587U, 140U}, {659U, 140U}, {784U, 240U},
  {0U, 130U}, {659U, 140U}, {784U, 140U}, {880U, 180U},
  {1175U, 260U}, {0U, 70U}, {1047U, 140U}, {880U, 140U},
  {784U, 220U}, {0U, 90U}, {659U, 140U}, {784U, 140U},
  {988U, 240U}, {0U, 130U}, {784U, 160U}, {659U, 160U},
  {587U, 180U}, {523U, 220U}, {0U, 70U}, {587U, 140U},
  {659U, 140U}, {784U, 220U}, {0U, 90U}, {880U, 140U},
  {784U, 140U}, {659U, 240U}, {0U, 130U}, {523U, 140U},
  {659U, 140U}, {784U, 180U}, {988U, 220U}, {1047U, 300U},
  {0U, 100U}, {784U, 160U}, {659U, 160U}, {587U, 200U},
  {523U, 420U}, {0U, 650U}
};
static const Nota_t musica_pausa[] = {
  {220U, 260U}, {262U, 260U}, {330U, 420U}, {0U, 120U},
  {294U, 260U}, {262U, 260U}, {220U, 420U}, {0U, 160U},
  {196U, 260U}, {220U, 260U}, {262U, 420U}, {0U, 120U},
  {247U, 260U}, {220U, 260U}, {196U, 500U}, {0U, 300U},
  {220U, 300U}, {330U, 300U}, {294U, 300U}, {262U, 500U},
  {0U, 200U}, {196U, 300U}, {247U, 300U}, {220U, 600U},
  {0U, 500U}
};
static const Nota_t efecto_aterrizaje[] = {
  {262U, 35U}, {196U, 65U}
};
static const char *const nombres_boss[4] = {
  "CHANCLA", "BRAVA", "ALEMANIA", "GRASA"
};
static const uint16_t colores_boss_claro[4] = {
  0x8010U, 0xF800U, 0xFB56U, 0xFFE0U
};
static const uint16_t colores_boss_oscuro[4] = {
  0x4008U, 0x7800U, 0xA18FU, 0xA500U
};
static const uint16_t sprite_idle_derecha[JUGADOR_ANCHO * JUGADOR_ALTO] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xf800, 0xf800, 0x0000, 0x0000,
  0xf800, 0xf800, 0xf800, 0x0000, 0xfe38, 0xf800, 0xf800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800,
  0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0xfe38, 0xfe38, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000
};
static const uint16_t sprite_carga_derecha[JUGADOR_ANCHO * JUGADOR_ALTO] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xf800, 0xf800, 0xf800, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0xf800, 0xf800, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0xf800, 0xf800, 0xf800, 0x0000, 0xfe38, 0x6800, 0x6800, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0xf800, 0xf800, 0xf800, 0xfe38, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};
static const uint16_t sprite_aire_derecha[JUGADOR_ANCHO * JUGADOR_ALTO] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0xf800, 0x0000, 0x0000, 0xf800,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xf800, 0xf800, 0xf800, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xf800, 0x0000, 0xf800, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0xf800, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xfe38, 0xfe38, 0x6800, 0xfe38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x6800, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x6800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};
static const uint16_t colores_pantalla[TOTAL_PANTALLAS] = {
  0x10A6U, 0x2149U, 0x3128U, 0x1803U,
  0x1003U, 0x0328U, 0x03CBU, 0x046DU,
  0x0183U, 0x3006U, 0x4008U, 0x600CU,
  0x8010U, 0x2004U, 0x0801U, 0x1800U
};
static const Plataforma_t plataformas[TOTAL_PANTALLAS][PLATAFORMAS_PANTALLA] = {
  {
    {25, 170, 90, 8},
    {170, 135, 90, 8},
    {60, 95, 80, 8},
    {200, 60, 90, 8},
    {105, 25, 85, 8}
  },
  {
    {220, 205, 90, 8},
    {20, 160, 85, 8},
    {145, 120, 80, 8},
    {245, 75, 65, 8},
    {170, 30, 90, 8}
  },
  {
    {20, 205, 110, 8},
    {35, 165, 90, 8},
    {180, 120, 85, 8},
    {70, 75, 85, 8},
    {205, 30, 95, 8}
  },
  {
    {20, 205, 110, 8},
    {90, 165, 80, 8},
    {15, 120, 75, 8},
    {160, 75, 90, 8},
    {45, 30, 90, 8}
  },
  {
    {200, 205, 110, 8},
    {180, 165, 90, 8},
    {65, 120, 85, 8},
    {205, 75, 85, 8},
    {100, 30, 100, 8}
  },
  {
    {20, 205, 110, 8},
    {145, 170, 90, 8},
    {235, 125, 70, 8},
    {95, 80, 95, 8},
    {210, 30, 90, 8}
  },
  {
    {20, 205, 110, 8},
    {65, 160, 85, 8},
    {190, 120, 100, 8},
    {30, 75, 90, 8},
    {135, 30, 95, 8}
  },
  {
    {220, 205, 90, 8},
    {165, 165, 80, 8},
    {35, 120, 100, 8},
    {205, 75, 95, 8},
    {80, 30, 100, 8}
  },
  {
    {210, 205, 100, 8},
    {40, 165, 90, 8},
    {175, 120, 85, 8},
    {65, 75, 90, 8},
    {205, 30, 95, 8}
  },
  {
    {20, 205, 110, 8},
    {100, 165, 75, 8},
    {215, 120, 90, 8},
    {45, 75, 100, 8},
    {155, 30, 95, 8}
  },
  {
    {220, 205, 90, 8},
    {180, 165, 85, 8},
    {55, 120, 95, 8},
    {210, 75, 90, 8},
    {90, 30, 100, 8}
  },
  {
    {20, 205, 110, 8},
    {145, 170, 100, 8},
    {30, 125, 85, 8},
    {195, 80, 105, 8},
    {95, 30, 100, 8}
  },
  {
    {215, 205, 95, 8},
    {120, 165, 80, 8},
    {25, 120, 90, 8},
    {155, 75, 95, 8},
    {40, 30, 100, 8}
  },
  {
    {20, 205, 105, 8},
    {180, 165, 90, 8},
    {70, 120, 85, 8},
    {215, 75, 85, 8},
    {110, 30, 95, 8}
  },
  {
    {210, 205, 100, 8},
    {45, 165, 90, 8},
    {175, 120, 90, 8},
    {65, 75, 85, 8},
    {200, 30, 100, 8}
  },
  {
    {20, 205, 110, 8},
    {155, 165, 90, 8},
    {35, 120, 85, 8},
    {195, 75, 100, 8},
    {90, 30, 100, 8}
  }
};
static const uint8_t fuente_5x7[26][5] = {
  {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU},
  {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U},
  {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U},
  {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU},
  {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U},
  {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U},
  {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU},
  {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU},
  {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U},
  {0x20U, 0x40U, 0x41U, 0x3FU, 0x01U},
  {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U},
  {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U},
  {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU},
  {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU},
  {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU},
  {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U},
  {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU},
  {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U},
  {0x46U, 0x49U, 0x49U, 0x49U, 0x31U},
  {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U},
  {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU},
  {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU},
  {0x3FU, 0x40U, 0x38U, 0x40U, 0x3FU},
  {0x63U, 0x14U, 0x08U, 0x14U, 0x63U},
  {0x07U, 0x08U, 0x70U, 0x08U, 0x07U},
  {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM10_Init(void);
static void MX_UART4_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
static void Audio_Tono(uint16_t frecuencia);
static void Audio_Detener(void);
static void Audio_Init(void);
static void Audio_MenuIniciar(void);
static void Audio_PausaIniciar(void);
static void Audio_EfectoAterrizaje(void);
static void Audio_Update(void);
static inline void LCD_Write8(uint8_t value);
static void LCD_HardwareReset(void);
static void LCD_WriteCommand(uint8_t command);
static void LCD_WriteCommandData(uint8_t command, const uint8_t *data, uint32_t length);
static void LCD_InitPantalla(void);
static void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
static void LCD_Llenar_Rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color);
static void LCD_Llenar_Pantalla(uint16_t color);
static void LCD_DibujarCaracter(int16_t x, int16_t y, char caracter, uint8_t escala,
                                uint16_t color, uint16_t fondo);
static void LCD_DibujarTexto(int16_t x, int16_t y, const char *texto, uint8_t escala,
                            uint16_t color, uint16_t fondo);
static uint16_t LCD_AnchoTexto(const char *texto, uint8_t escala);
static void LCD_Apagar(void);
static void LCD_Encender(void);

static void Control_Iniciar(void);
static uint8_t Control_Leer(void);
static bool Control_Presionado(uint8_t estado, uint8_t boton);
static void Botones_Sincronizar(void);
static void Menu_Dibujar(void);
static void Menu_Init(void);
static void Menu_Update(void);
static void Load_Dibujar(void);
static void Load_Init(void);
static void Load_Update(void);
static void Pausa_Dibujar(void);
static void Pausa_Init(void);
static void Pausa_Continuar(void);
static void Pausa_Update(void);
static void Dormido_Update(void);
static bool Game_TraslapeHorizontal(int16_t x, const Plataforma_t *plataforma);
static bool Game_TieneSoporte(int16_t x, int16_t y);
static int16_t Game_ResolverHorizontal(int16_t old_x, int16_t new_x, int16_t y);
static int16_t Game_ResolverVertical(int16_t old_y, int16_t new_y);
static bool Game_EsPantallaBoss(void);
static uint8_t Game_IndiceBoss(void);
static bool Game_BossActivo(void);
static bool Game_PlataformasActivas(void);
static uint8_t Game_VidaMaximaBoss(void);
static void Game_BossInit(void);
static void Game_LimpiarDisparosBoss(void);
static void Game_CrearDisparoBoss(void);
static void Game_DibujarDisparosBoss(void);
static bool Game_ActualizarDisparosBoss(void);
static bool Game_ActualizarBoss(bool disparar, bool *golpeado);
static bool Game_JugadorTocaPistola(int16_t x, int16_t y);
static bool Game_DisparoTocaPlataforma(void);
static void Game_IniciarDisparo(void);
static void Game_ActualizarDisparo(bool disparar);
static void Game_DibujarPlataforma(const Plataforma_t *plataforma);
static void Game_DibujarBoss(void);
static void Game_DibujarVidaBoss(void);
static void Game_DibujarVidaJugador(void);
static void Game_DibujarDisparo(void);
static void Game_DibujarPistola(void);
static void Game_BorrarDisparo(int16_t x, int16_t y);
static bool SD_Init(void);
static void SD_CerrarFondo(void);
static bool SD_AbrirFondo(uint8_t indice);
static bool SD_DibujarRegion(uint8_t indice, int16_t x, int16_t y,
                            int16_t ancho, int16_t alto);
static uint8_t Game_IndiceFondo(void);
static void Game_RestaurarFondo(int16_t x, int16_t y, int16_t ancho, int16_t alto);
static void Game_DibujarFondo(void);
static const uint16_t *Game_SpriteJugador(void);
static void LCD_DibujarSpriteJugador(int16_t x, int16_t y,
                                     const uint16_t *sprite, bool derecha);
static void Game_DibujarJugador(int16_t x, int16_t y);
static void Game_BorrarJugador(int16_t x, int16_t y);
static void Game_Iniciar(uint8_t pantalla_inicial, bool pistola_inicial);
static void Game_Init(void);
static void Game_InitBoss(uint8_t indice);
static void Game_Update(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void Audio_Tono(uint16_t frecuencia)
{
  uint32_t periodo;

  if (frecuencia == 0U)
  {
    __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, 0U);
    return;
  }

  periodo = AUDIO_TIMER_HZ / frecuencia;
  if (periodo < 2U)
  {
    periodo = 2U;
  }

  __HAL_TIM_SET_AUTORELOAD(&htim10, periodo - 1U);
  __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, periodo / 2U);
  __HAL_TIM_SET_COUNTER(&htim10, 0U);
  htim10.Instance->EGR = TIM_EGR_UG;
}

static void Audio_Detener(void)
{
  audio_estado = AUDIO_SILENCIO;
  audio_indice = 0U;
  audio_siguiente = 0U;
  Audio_Tono(0U);
}

static void Audio_Init(void)
{
  if (HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  Audio_Detener();
}

static void Audio_MenuIniciar(void)
{
  audio_estado = AUDIO_MENU;
  audio_indice = 0U;
  Audio_Tono(musica_menu[0].frecuencia);
  audio_siguiente = HAL_GetTick() + musica_menu[0].duracion;
}

static void Audio_PausaIniciar(void)
{
  audio_estado = AUDIO_PAUSA;
  audio_indice = 0U;
  Audio_Tono(musica_pausa[0].frecuencia);
  audio_siguiente = HAL_GetTick() + musica_pausa[0].duracion;
}

static void Audio_EfectoAterrizaje(void)
{
  audio_estado = AUDIO_ATERRIZAJE;
  audio_indice = 0U;
  Audio_Tono(efecto_aterrizaje[0].frecuencia);
  audio_siguiente = HAL_GetTick() + efecto_aterrizaje[0].duracion;
}

static void Audio_Update(void)
{
  const Nota_t *nota;
  uint32_t ahora;

  if (audio_estado == AUDIO_SILENCIO)
  {
    return;
  }

  ahora = HAL_GetTick();
  if ((int32_t)(ahora - audio_siguiente) < 0)
  {
    return;
  }

  audio_indice++;

  if (audio_estado == AUDIO_MENU)
  {
    if (audio_indice >= (sizeof(musica_menu) / sizeof(musica_menu[0])))
    {
      audio_indice = 0U;
    }
    nota = &musica_menu[audio_indice];
  }
  else if (audio_estado == AUDIO_PAUSA)
  {
    if (audio_indice >= (sizeof(musica_pausa) / sizeof(musica_pausa[0])))
    {
      audio_indice = 0U;
    }
    nota = &musica_pausa[audio_indice];
  }
  else
  {
    if (audio_indice >= (sizeof(efecto_aterrizaje) / sizeof(efecto_aterrizaje[0])))
    {
      Audio_Detener();
      return;
    }
    nota = &efecto_aterrizaje[audio_indice];
  }

  Audio_Tono(nota->frecuencia);
  audio_siguiente = ahora + nota->duracion;
}

//Bus entre tres puertos
static inline void LCD_Write8(uint8_t value)
{
  uint32_t set_a = 0U;
  uint32_t set_b = 0U;
  uint32_t set_c = 0U;

  if ((value & 0x01U) != 0U) set_a |= LCD_D0_Pin;
  if ((value & 0x04U) != 0U) set_a |= LCD_D2_Pin;
  if ((value & 0x80U) != 0U) set_a |= LCD_D7_Pin;

  if ((value & 0x08U) != 0U) set_b |= LCD_D3_Pin;
  if ((value & 0x10U) != 0U) set_b |= LCD_D4_Pin;
  if ((value & 0x20U) != 0U) set_b |= LCD_D5_Pin;
  if ((value & 0x40U) != 0U) set_b |= LCD_D6_Pin;

  if ((value & 0x02U) != 0U) set_c |= LCD_D1_Pin;

  GPIOA->BSRR = (((uint32_t)LCD_DATA_A_MASK & ~set_a) << 16U) | set_a;
  GPIOB->BSRR = (((uint32_t)LCD_DATA_B_MASK & ~set_b) << 16U) | set_b;
  GPIOC->BSRR = (((uint32_t)LCD_DATA_C_MASK & ~set_c) << 16U) | set_c;

  LCD_WR_LOW();
  __NOP();
  __NOP();
  __NOP();
  LCD_WR_HIGH();
}

static void LCD_HardwareReset(void)
{
  LCD_CS_HIGH();
  LCD_RD_HIGH();
  LCD_WR_HIGH();

  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(5U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(150U);
}

static void LCD_WriteCommand(uint8_t command)
{
  LCD_CS_LOW();
  LCD_RS_COMMAND();
  LCD_Write8(command);
  LCD_CS_HIGH();
}

static void LCD_WriteCommandData(uint8_t command, const uint8_t *data, uint32_t length)
{
  LCD_CS_LOW();
  LCD_RS_COMMAND();
  LCD_Write8(command);
  LCD_RS_DATA();

  for (uint32_t i = 0U; i < length; i++)
  {
    LCD_Write8(data[i]);
  }

  LCD_CS_HIGH();
}

static void LCD_InitPantalla(void)
{
  static const uint8_t power_b[] = {0x00U, 0xC1U, 0x30U};
  static const uint8_t power_seq[] = {0x64U, 0x03U, 0x12U, 0x81U};
  static const uint8_t driver_timing_a[] = {0x85U, 0x00U, 0x78U};
  static const uint8_t power_a[] = {0x39U, 0x2CU, 0x00U, 0x34U, 0x02U};
  static const uint8_t pump_ratio[] = {0x20U};
  static const uint8_t driver_timing_b[] = {0x00U, 0x00U};
  static const uint8_t power_control_1[] = {0x23U};
  static const uint8_t power_control_2[] = {0x10U};
  static const uint8_t vcom_control_1[] = {0x3EU, 0x28U};
  static const uint8_t vcom_control_2[] = {0x86U};
  static const uint8_t memory_access[] = {0x28U};
  static const uint8_t pixel_format[] = {0x55U};
  static const uint8_t frame_rate[] = {0x00U, 0x18U};
  static const uint8_t display_function[] = {0x08U, 0x82U, 0x27U};
  static const uint8_t gamma_disable[] = {0x00U};
  static const uint8_t gamma_curve[] = {0x01U};
  static const uint8_t gamma_positive[] = {
    0x0FU, 0x31U, 0x2BU, 0x0CU, 0x0EU, 0x08U, 0x4EU, 0xF1U,
    0x37U, 0x07U, 0x10U, 0x03U, 0x0EU, 0x09U, 0x00U
  };
  static const uint8_t gamma_negative[] = {
    0x00U, 0x0EU, 0x14U, 0x03U, 0x11U, 0x07U, 0x31U, 0xC1U,
    0x48U, 0x08U, 0x0FU, 0x0CU, 0x31U, 0x36U, 0x0FU
  };

  LCD_WriteCommand(0x01U);
  HAL_Delay(5U);
  LCD_WriteCommand(0x28U);

  LCD_WriteCommandData(0xCFU, power_b, sizeof(power_b));
  LCD_WriteCommandData(0xEDU, power_seq, sizeof(power_seq));
  LCD_WriteCommandData(0xE8U, driver_timing_a, sizeof(driver_timing_a));
  LCD_WriteCommandData(0xCBU, power_a, sizeof(power_a));
  LCD_WriteCommandData(0xF7U, pump_ratio, sizeof(pump_ratio));
  LCD_WriteCommandData(0xEAU, driver_timing_b, sizeof(driver_timing_b));
  LCD_WriteCommandData(0xC0U, power_control_1, sizeof(power_control_1));
  LCD_WriteCommandData(0xC1U, power_control_2, sizeof(power_control_2));
  LCD_WriteCommandData(0xC5U, vcom_control_1, sizeof(vcom_control_1));
  LCD_WriteCommandData(0xC7U, vcom_control_2, sizeof(vcom_control_2));
  LCD_WriteCommandData(0x36U, memory_access, sizeof(memory_access));
  LCD_WriteCommandData(0x3AU, pixel_format, sizeof(pixel_format));
  LCD_WriteCommandData(0xB1U, frame_rate, sizeof(frame_rate));
  LCD_WriteCommandData(0xB6U, display_function, sizeof(display_function));
  LCD_WriteCommandData(0xF2U, gamma_disable, sizeof(gamma_disable));
  LCD_WriteCommandData(0x26U, gamma_curve, sizeof(gamma_curve));
  LCD_WriteCommandData(0xE0U, gamma_positive, sizeof(gamma_positive));
  LCD_WriteCommandData(0xE1U, gamma_negative, sizeof(gamma_negative));

  LCD_WriteCommand(0x11U);
  HAL_Delay(120U);
  LCD_WriteCommand(0x29U);
  HAL_Delay(20U);
}

static void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  uint8_t column_data[4] = {
    (uint8_t)(x0 >> 8U), (uint8_t)x0,
    (uint8_t)(x1 >> 8U), (uint8_t)x1
  };
  uint8_t page_data[4] = {
    (uint8_t)(y0 >> 8U), (uint8_t)y0,
    (uint8_t)(y1 >> 8U), (uint8_t)y1
  };

  LCD_WriteCommandData(0x2AU, column_data, sizeof(column_data));
  LCD_WriteCommandData(0x2BU, page_data, sizeof(page_data));
  LCD_WriteCommand(0x2CU);
}

static void LCD_Llenar_Rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
  if ((width <= 0) || (height <= 0) || (x >= LCD_ANCHO) || (y >= LCD_ALTO))
  {
    return;
  }

  if (x < 0)
  {
    width = (int16_t)(width + x);
    x = 0;
  }

  if (y < 0)
  {
    height = (int16_t)(height + y);
    y = 0;
  }

  if ((x + width) > LCD_ANCHO)
  {
    width = (int16_t)(LCD_ANCHO - x);
  }

  if ((y + height) > LCD_ALTO)
  {
    height = (int16_t)(LCD_ALTO - y);
  }

  if ((width <= 0) || (height <= 0))
  {
    return;
  }

  LCD_SetAddressWindow((uint16_t)x, (uint16_t)y,
                       (uint16_t)(x + width - 1),
                       (uint16_t)(y + height - 1));

  uint8_t color_high = (uint8_t)(color >> 8U);
  uint8_t color_low = (uint8_t)color;
  uint32_t pixel_count = (uint32_t)width * (uint32_t)height;

  LCD_CS_LOW();
  LCD_RS_DATA();

  while (pixel_count > 0U)
  {
    LCD_Write8(color_high);
    LCD_Write8(color_low);
    pixel_count--;
  }

  LCD_CS_HIGH();
}

static void LCD_Llenar_Pantalla(uint16_t color)
{
  LCD_Llenar_Rect(0, 0, LCD_ANCHO, LCD_ALTO, color);
}

static void LCD_DibujarCaracter(int16_t x, int16_t y, char caracter, uint8_t escala,
                                uint16_t color, uint16_t fondo)
{
  const uint8_t *forma = NULL;

  LCD_Llenar_Rect(x, y, (int16_t)(6U * escala), (int16_t)(7U * escala), fondo);

  if ((caracter >= 'a') && (caracter <= 'z'))
  {
    caracter = (char)(caracter - 'a' + 'A');
  }

  if ((caracter >= 'A') && (caracter <= 'Z'))
  {
    forma = fuente_5x7[(uint8_t)(caracter - 'A')];
  }

  if (caracter == '-')
  {
    LCD_Llenar_Rect(x, (int16_t)(y + (int16_t)(3U * escala)),
                    (int16_t)(5U * escala), escala, color);
    return;
  }

  if (forma == NULL)
  {
    return;
  }

  for (uint8_t columna = 0U; columna < 5U; columna++)
  {
    for (uint8_t fila = 0U; fila < 7U; fila++)
    {
      if ((forma[columna] & (1U << fila)) != 0U)
      {
        LCD_Llenar_Rect((int16_t)(x + (int16_t)(columna * escala)),
                        (int16_t)(y + (int16_t)(fila * escala)),
                        escala, escala, color);
      }
    }
  }
}

static void LCD_DibujarTexto(int16_t x, int16_t y, const char *texto, uint8_t escala,
                            uint16_t color, uint16_t fondo)
{
  while (*texto != '\0')
  {
    LCD_DibujarCaracter(x, y, *texto, escala, color, fondo);
    x = (int16_t)(x + (int16_t)(6U * escala));
    texto++;
  }
}

static uint16_t LCD_AnchoTexto(const char *texto, uint8_t escala)
{
  uint16_t caracteres = (uint16_t)strlen(texto);

  if (caracteres == 0U)
  {
    return 0U;
  }

  return (uint16_t)((caracteres * 6U * escala) - escala);
}

static void LCD_Apagar(void)
{
  LCD_Llenar_Pantalla(COLOR_NEGRO);
  LCD_WriteCommand(0x28U);
  HAL_Delay(20U);
  LCD_WriteCommand(0x10U);
  HAL_Delay(120U);
}

static void LCD_Encender(void)
{
  LCD_WriteCommand(0x11U);
  HAL_Delay(120U);
  LCD_WriteCommand(0x29U);
  HAL_Delay(20U);
}

static void Control_Iniciar(void)
{
  control_estado = 0U;
  control_ultimo_ms = HAL_GetTick();
  HAL_UART_Receive_IT(&huart4, &control_rx_byte, 1U);
}

static uint8_t Control_Leer(void)
{
  if ((HAL_GetTick() - control_ultimo_ms) > CONTROL_TIMEOUT_MS)
  {
    return 0U;
  }

  return control_estado;
}

static bool Control_Presionado(uint8_t estado, uint8_t boton)
{
  return (estado & boton) != 0U;
}

static void Botones_Sincronizar(void)
{
  uint8_t control = Control_Leer();

  left_anterior = Control_Presionado(control, CONTROL_IZQUIERDA);
  right_anterior = Control_Presionado(control, CONTROL_DERECHA);
  jump_anterior = Control_Presionado(control, CONTROL_SALTO);
  shoot_anterior = Control_Presionado(control, CONTROL_DISPARO);
  pause_anterior = Control_Presionado(control, CONTROL_PAUSA);
}

static void Menu_Dibujar(void)
{
  const char titulo[] = "EX-Machina";
  const char empezar[] = "EMPEZAR";
  const char load[] = "LOAD";
  const char salir[] = "SALIR";
  uint16_t color_empezar = (opcion_menu == MENU_EMPEZAR) ? COLOR_SELECCION : COLOR_OPCION;
  uint16_t color_load = (opcion_menu == MENU_LOAD) ? COLOR_SELECCION : COLOR_OPCION;
  uint16_t color_salir = (opcion_menu == MENU_SALIR) ? COLOR_SELECCION : COLOR_OPCION;
  int16_t titulo_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(titulo, 4U)) / 2U);
  int16_t empezar_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(empezar, 3U)) / 2U);
  int16_t load_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(load, 3U)) / 2U);
  int16_t salir_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(salir, 3U)) / 2U);

  LCD_Llenar_Pantalla(COLOR_MENU);
  LCD_DibujarTexto(titulo_x, 10, titulo, 4U, COLOR_TITULO, COLOR_MENU);
  LCD_Llenar_Rect(45, 48, 230, 3, COLOR_TITULO);

  LCD_Llenar_Rect(65, 67, 190, 36, color_empezar);
  LCD_DibujarTexto(empezar_x, 75, empezar, 3U,
                   (opcion_menu == MENU_EMPEZAR) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_empezar);

  LCD_Llenar_Rect(65, 117, 190, 36, color_load);
  LCD_DibujarTexto(load_x, 125, load, 3U,
                   (opcion_menu == MENU_LOAD) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_load);

  LCD_Llenar_Rect(65, 167, 190, 36, color_salir);
  LCD_DibujarTexto(salir_x, 175, salir, 3U,
                   (opcion_menu == MENU_SALIR) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_salir);
}

static void Menu_Init(void)
{
  SD_CerrarFondo();
  estado_actual = ESTADO_MENU;
  opcion_menu = MENU_EMPEZAR;
  despertar_armado = false;
  Menu_Dibujar();
  Botones_Sincronizar();
  Audio_MenuIniciar();
}

static void Menu_Update(void)
{
  uint8_t control = Control_Leer();
  bool left_actual = Control_Presionado(control, CONTROL_IZQUIERDA);
  bool right_actual = Control_Presionado(control, CONTROL_DERECHA);
  bool jump_actual = Control_Presionado(control, CONTROL_SALTO);
  bool mover_izquierda = left_actual && !left_anterior;
  bool mover_derecha = right_actual && !right_anterior;
  bool confirmar = jump_actual && !jump_anterior;

  left_anterior = left_actual;
  right_anterior = right_actual;
  jump_anterior = jump_actual;

  if (mover_izquierda)
  {
    opcion_menu = (opcion_menu == 0U) ?
                  (MENU_TOTAL - 1U) : (opcion_menu - 1U);
    Menu_Dibujar();
  }
  else if (mover_derecha)
  {
    opcion_menu = (uint8_t)((opcion_menu + 1U) % MENU_TOTAL);
    Menu_Dibujar();
  }

  if (confirmar)
  {
    if (opcion_menu == MENU_EMPEZAR)
    {
      Audio_Detener();
      estado_actual = ESTADO_JUEGO;
      Game_Init();
      jump_apachado = jump_actual;
    }
    else if (opcion_menu == MENU_LOAD)
    {
      Load_Init();
    }
    else
    {
      Audio_Detener();
      LCD_Apagar();
      estado_actual = ESTADO_DORMIDO;
      despertar_armado = false;
    }
  }
}

static void Load_Dibujar(void)
{
  const char titulo[] = "SELECCIONAR BOSS";
  static const int16_t posiciones[TOTAL_BOSSES] = {50, 94, 138, 182};
  int16_t titulo_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(titulo, 3U)) / 2U);

  LCD_Llenar_Pantalla(COLOR_MENU);
  LCD_DibujarTexto(titulo_x, 10, titulo, 3U, COLOR_TITULO, COLOR_MENU);
  LCD_Llenar_Rect(18, 38, 284, 3, COLOR_TITULO);

  for (uint8_t i = 0U; i < TOTAL_BOSSES; i++)
  {
    uint16_t fondo = (opcion_load == i) ?
                     colores_boss_claro[i] : COLOR_OPCION;
    uint16_t texto = (opcion_load == i) ? COLOR_NEGRO : COLOR_BLANCO;
    int16_t texto_x = (int16_t)((LCD_ANCHO -
                      LCD_AnchoTexto(nombres_boss[i], 2U)) / 2U);

    LCD_Llenar_Rect(45, posiciones[i], 230, 32, fondo);
    LCD_DibujarTexto(texto_x, posiciones[i] + 9,
                     nombres_boss[i], 2U, texto, fondo);
  }
}

static void Load_Init(void)
{
  estado_actual = ESTADO_LOAD;
  opcion_load = 0U;
  Load_Dibujar();
  Botones_Sincronizar();
}

static void Load_Update(void)
{
  uint8_t control = Control_Leer();
  bool left_actual = Control_Presionado(control, CONTROL_IZQUIERDA);
  bool right_actual = Control_Presionado(control, CONTROL_DERECHA);
  bool jump_actual = Control_Presionado(control, CONTROL_SALTO);
  bool pause_actual = Control_Presionado(control, CONTROL_PAUSA);
  bool mover_izquierda = left_actual && !left_anterior;
  bool mover_derecha = right_actual && !right_anterior;
  bool confirmar = jump_actual && !jump_anterior;
  bool volver = pause_actual && !pause_anterior;

  left_anterior = left_actual;
  right_anterior = right_actual;
  jump_anterior = jump_actual;
  pause_anterior = pause_actual;

  if (volver)
  {
    Menu_Init();
    return;
  }

  if (mover_izquierda)
  {
    opcion_load = (opcion_load == 0U) ?
                  (TOTAL_BOSSES - 1U) : (opcion_load - 1U);
    Load_Dibujar();
  }
  else if (mover_derecha)
  {
    opcion_load = (uint8_t)((opcion_load + 1U) % TOTAL_BOSSES);
    Load_Dibujar();
  }

  if (confirmar)
  {
    Audio_Detener();
    estado_actual = ESTADO_JUEGO;
    Game_InitBoss(opcion_load);
    jump_apachado = jump_actual;
  }
}

static void Pausa_Dibujar(void)
{
  const char titulo[] = "PAUSA";
  const char continuar[] = "CONTINUAR";
  const char salir[] = "SALIR";
  uint16_t color_continuar = (opcion_pausa == PAUSA_CONTINUAR) ?
                             COLOR_SELECCION : COLOR_OPCION;
  uint16_t color_salir = (opcion_pausa == PAUSA_SALIR) ?
                         COLOR_SELECCION : COLOR_OPCION;
  int16_t titulo_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(titulo, 4U)) / 2U);
  int16_t continuar_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(continuar, 3U)) / 2U);
  int16_t salir_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(salir, 3U)) / 2U);

  LCD_Llenar_Pantalla(COLOR_MENU);
  LCD_DibujarTexto(titulo_x, 18, titulo, 4U, COLOR_TITULO, COLOR_MENU);
  LCD_Llenar_Rect(45, 58, 230, 3, COLOR_TITULO);

  LCD_Llenar_Rect(55, 92, 210, 39, color_continuar);
  LCD_DibujarTexto(continuar_x, 101, continuar, 3U,
                   (opcion_pausa == PAUSA_CONTINUAR) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_continuar);

  LCD_Llenar_Rect(55, 148, 210, 39, color_salir);
  LCD_DibujarTexto(salir_x, 157, salir, 3U,
                   (opcion_pausa == PAUSA_SALIR) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_salir);
}

static void Pausa_Init(void)
{
  estado_actual = ESTADO_PAUSA;
  opcion_pausa = PAUSA_CONTINUAR;
  pausa_inicio = HAL_GetTick();
  Audio_PausaIniciar();
  Pausa_Dibujar();
  Botones_Sincronizar();
}

static void Pausa_Continuar(void)
{
  uint32_t duracion = HAL_GetTick() - pausa_inicio;

  Audio_Detener();
  salto_inicio += duracion;
  proximo_frame_caminata += duracion;
  proximo_disparo_boss += duracion;
  estado_actual = ESTADO_JUEGO;
  Game_DibujarFondo();
  Game_DibujarJugador(jugador.x, (int16_t)(jugador.y_fp >> FP_SHIFT));
  Botones_Sincronizar();
  jump_apachado = jump_anterior;
}

static void Pausa_Update(void)
{
  uint8_t control = Control_Leer();
  bool left_actual = Control_Presionado(control, CONTROL_IZQUIERDA);
  bool right_actual = Control_Presionado(control, CONTROL_DERECHA);
  bool jump_actual = Control_Presionado(control, CONTROL_SALTO);
  bool pause_actual = Control_Presionado(control, CONTROL_PAUSA);
  bool mover_opcion = (left_actual && !left_anterior) ||
                      (right_actual && !right_anterior);
  bool confirmar = jump_actual && !jump_anterior;
  bool continuar = pause_actual && !pause_anterior;

  left_anterior = left_actual;
  right_anterior = right_actual;
  jump_anterior = jump_actual;
  pause_anterior = pause_actual;

  if (mover_opcion)
  {
    opcion_pausa = (opcion_pausa == PAUSA_CONTINUAR) ?
                   PAUSA_SALIR : PAUSA_CONTINUAR;
    Pausa_Dibujar();
  }

  if (continuar || (confirmar && (opcion_pausa == PAUSA_CONTINUAR)))
  {
    Pausa_Continuar();
  }
  else if (confirmar && (opcion_pausa == PAUSA_SALIR))
  {
    Menu_Init();
  }
}

static void Dormido_Update(void)
{
  bool algun_boton = Control_Leer() != 0U;

  if (!despertar_armado)
  {
    if (!algun_boton)
    {
      despertar_armado = true;
    }
    return;
  }

  if (algun_boton)
  {
    LCD_Encender();
    Menu_Init();
  }
}

static bool Game_TraslapeHorizontal(int16_t x, const Plataforma_t *plataforma)
{
  return ((x + JUGADOR_ANCHO) > plataforma->x) &&
         (x < (plataforma->x + plataforma->ancho));
}

static bool Game_TieneSoporte(int16_t x, int16_t y)
{
  int16_t pies = y + JUGADOR_ALTO;

  if (((pantalla_actual == 0U) || Game_EsPantallaBoss()) &&
      (pies == PISO_Y))
  {
    return true;
  }

  if (Game_PlataformasActivas())
  {
    for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
    {
      const Plataforma_t *plataforma = &plataformas[pantalla_actual][i];

      if ((pies == plataforma->y) && Game_TraslapeHorizontal(x, plataforma))
      {
        return true;
      }
    }
  }

  return false;
}

static int16_t Game_ResolverHorizontal(int16_t old_x, int16_t new_x, int16_t y)
{
  if (!Game_PlataformasActivas())
  {
    return new_x;
  }

  for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
  {
    const Plataforma_t *plataforma = &plataformas[pantalla_actual][i];
    int16_t derecha = plataforma->x + plataforma->ancho;
    bool traslape_vertical = ((y + JUGADOR_ALTO) > plataforma->y) &&
                             (y < (plataforma->y + plataforma->alto));

    if (!traslape_vertical)
    {
      continue;
    }

    if ((new_x > old_x) &&
        ((old_x + JUGADOR_ANCHO) <= plataforma->x) &&
        ((new_x + JUGADOR_ANCHO) >= plataforma->x))
    {
      new_x = plataforma->x - JUGADOR_ANCHO;
    }
    else if ((new_x < old_x) &&
             (old_x >= derecha) &&
             (new_x <= derecha))
    {
      new_x = derecha;
    }
  }

  return new_x;
}

static int16_t Game_ResolverVertical(int16_t old_y, int16_t new_y)
{
  if (new_y > old_y)
  {
    int16_t superficie = LCD_ALTO + 1;
    bool colision = false;

    if (Game_PlataformasActivas())
    {
      for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
      {
        const Plataforma_t *plataforma = &plataformas[pantalla_actual][i];

        if (Game_TraslapeHorizontal(jugador.x, plataforma) &&
            ((old_y + JUGADOR_ALTO) <= plataforma->y) &&
            ((new_y + JUGADOR_ALTO) >= plataforma->y) &&
            (plataforma->y < superficie))
        {
          superficie = plataforma->y;
          colision = true;
        }
      }
    }

    if (((pantalla_actual == 0U) || Game_EsPantallaBoss()) &&
        ((old_y + JUGADOR_ALTO) <= PISO_Y) &&
        ((new_y + JUGADOR_ALTO) >= PISO_Y) &&
        (PISO_Y < superficie))
    {
      superficie = PISO_Y;
      colision = true;
    }

    if (colision)
    {
      jugador.vy_fp = 0L;
      jugador.vx = 0;
      jugador.on_ground = true;
      return superficie - JUGADOR_ALTO;
    }
  }
  else if (new_y < old_y)
  {
    int16_t techo = -1;
    bool colision = false;

    if (Game_PlataformasActivas())
    {
      for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
      {
        const Plataforma_t *plataforma = &plataformas[pantalla_actual][i];
        int16_t parte_inferior = plataforma->y + plataforma->alto;

        if (Game_TraslapeHorizontal(jugador.x, plataforma) &&
            (old_y >= parte_inferior) &&
            (new_y <= parte_inferior) &&
            (parte_inferior > techo))
        {
          techo = parte_inferior;
          colision = true;
        }
      }
    }

    if (colision)
    {
      jugador.vy_fp = 0L;
      jugador.on_ground = false;
      return techo;
    }
  }

  return new_y;
}

static bool Game_EsPantallaBoss(void)
{
  return ((pantalla_actual + 1U) % 4U) == 0U;
}

static uint8_t Game_IndiceBoss(void)
{
  return pantalla_actual / 4U;
}

static bool Game_BossActivo(void)
{
  return Game_EsPantallaBoss() &&
         !bosses_derrotados[Game_IndiceBoss()];
}

static bool Game_PlataformasActivas(void)
{
  return !Game_BossActivo();
}

static uint8_t Game_VidaMaximaBoss(void)
{
  return (uint8_t)(3U + (Game_IndiceBoss() * 2U));
}

static void Game_BossInit(void)
{
  boss_x = (LCD_ANCHO - BOSS_ANCHO) / 2;
  boss_direccion = 1;
  boss_vida = Game_VidaMaximaBoss();
  vida_jugador = JUGADOR_VIDA_MAX;
  disparo_activo = false;
  disparo_dx = 0;
  disparo_dy = 0;
  Game_LimpiarDisparosBoss();
  patron_disparo_boss = 0U;
  proximo_disparo_boss = HAL_GetTick() + 500U;
}

static void Game_LimpiarDisparosBoss(void)
{
  memset(disparos_boss, 0, sizeof(disparos_boss));
}

static void Game_CrearDisparoBoss(void)
{
  static const int8_t direcciones[5] = {-2, 2, -1, 1, 0};

  for (uint8_t i = 0U; i < BOSS_DISPAROS_MAX; i++)
  {
    if (!disparos_boss[i].activo)
    {
      disparos_boss[i].x = boss_x + ((BOSS_ANCHO - BOSS_DISPARO_ANCHO) / 2);
      disparos_boss[i].y = BOSS_Y + BOSS_ALTO;
      disparos_boss[i].dx = direcciones[patron_disparo_boss % 5U];
      disparos_boss[i].dy = 3;
      disparos_boss[i].activo = true;
      patron_disparo_boss++;
      return;
    }
  }
}

static void Game_DibujarDisparosBoss(void)
{
  for (uint8_t i = 0U; i < BOSS_DISPAROS_MAX; i++)
  {
    if (disparos_boss[i].activo)
    {
      LCD_Llenar_Rect(disparos_boss[i].x, disparos_boss[i].y,
                      BOSS_DISPARO_ANCHO, BOSS_DISPARO_ALTO,
                      COLOR_JUGADOR);
    }
  }
}

static bool Game_ActualizarDisparosBoss(void)
{
  bool golpeado = false;
  int16_t jugador_y = (int16_t)(jugador.y_fp >> FP_SHIFT);
  uint16_t fondo = colores_pantalla[pantalla_actual];

  for (uint8_t i = 0U; i < BOSS_DISPAROS_MAX; i++)
  {
    DisparoBoss_t *disparo = &disparos_boss[i];

    if (!disparo->activo)
    {
      continue;
    }

    LCD_Llenar_Rect(disparo->x, disparo->y,
                    BOSS_DISPARO_ANCHO, BOSS_DISPARO_ALTO, fondo);
    disparo->x = (int16_t)(disparo->x + disparo->dx);
    disparo->y = (int16_t)(disparo->y + disparo->dy);

    if ((disparo->x >= LCD_ANCHO) ||
        ((disparo->x + BOSS_DISPARO_ANCHO) < 0) ||
        ((disparo->y + BOSS_DISPARO_ALTO) >= PISO_Y))
    {
      disparo->activo = false;
      continue;
    }

    if (((disparo->x + BOSS_DISPARO_ANCHO) > jugador.x) &&
        (disparo->x < (jugador.x + JUGADOR_ANCHO)) &&
        ((disparo->y + BOSS_DISPARO_ALTO) > jugador_y) &&
        (disparo->y < (jugador_y + JUGADOR_ALTO)))
    {
      disparo->activo = false;
      golpeado = true;
    }
  }

  if ((int32_t)(HAL_GetTick() - proximo_disparo_boss) >= 0)
  {
    Game_CrearDisparoBoss();
    proximo_disparo_boss = HAL_GetTick() + BOSS_DISPARO_MS;
  }

  Game_DibujarDisparosBoss();
  return golpeado;
}

static bool Game_JugadorTocaPistola(int16_t x, int16_t y)
{
  return (pantalla_actual == 0U) && !tiene_pistola &&
         ((x + JUGADOR_ANCHO) > PISTOLA_X) &&
         (x < (PISTOLA_X + PISTOLA_ANCHO)) &&
         ((y + JUGADOR_ALTO) > PISTOLA_Y) &&
         (y < (PISTOLA_Y + PISTOLA_ALTO));
}

static bool Game_DisparoTocaPlataforma(void)
{
  if (!Game_PlataformasActivas())
  {
    return false;
  }

  for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
  {
    const Plataforma_t *plataforma = &plataformas[pantalla_actual][i];

    if (((disparo_x + DISPARO_ANCHO) > plataforma->x) &&
        (disparo_x < (plataforma->x + plataforma->ancho)) &&
        ((disparo_y + DISPARO_ALTO) > plataforma->y) &&
        (disparo_y < (plataforma->y + plataforma->alto)))
    {
      return true;
    }
  }

  return false;
}

static void Game_IniciarDisparo(void)
{
  if (!tiene_pistola || disparo_activo)
  {
    return;
  }

  disparo_x = jugador.x + ((JUGADOR_ANCHO - DISPARO_ANCHO) / 2);
  disparo_y = (int16_t)((jugador.y_fp >> FP_SHIFT) - DISPARO_ALTO);
  disparo_dx = 0;
  disparo_dy = -DISPARO_SPEED;
  disparo_activo = true;
}

static void Game_DibujarPlataforma(const Plataforma_t *plataforma)
{
  LCD_Llenar_Rect(plataforma->x, plataforma->y,
                  plataforma->ancho, plataforma->alto, COLOR_PISO);
  LCD_Llenar_Rect(plataforma->x, plataforma->y,
                  plataforma->ancho, 2, COLOR_PISO_TOP);
}

static void Game_DibujarBoss(void)
{
  uint8_t indice;
  uint16_t color_claro;
  uint16_t color_oscuro;
  uint16_t fondo;

  if (!Game_BossActivo())
  {
    return;
  }

  indice = Game_IndiceBoss();
  color_claro = colores_boss_claro[indice];
  color_oscuro = colores_boss_oscuro[indice];
  fondo = colores_pantalla[pantalla_actual];

  if (indice == 0U)
  {
    LCD_Llenar_Rect(boss_x + 9, BOSS_Y, 14, 3, color_oscuro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 3, 20, 4, color_oscuro);
    LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 7, 24, 21, color_oscuro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 28, 20, 5, color_oscuro);
    LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 33, 14, 3, color_oscuro);
    LCD_Llenar_Rect(boss_x + 8, BOSS_Y + 4, 16, 3, color_claro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 8, 20, 19, color_claro);
    LCD_Llenar_Rect(boss_x + 8, BOSS_Y + 27, 16, 4, color_claro);
    LCD_Llenar_Rect(boss_x + 7, BOSS_Y + 9, 4, 7, color_oscuro);
    LCD_Llenar_Rect(boss_x + 10, BOSS_Y + 13, 4, 7, color_oscuro);
    LCD_Llenar_Rect(boss_x + 21, BOSS_Y + 9, 4, 7, color_oscuro);
    LCD_Llenar_Rect(boss_x + 18, BOSS_Y + 13, 4, 7, color_oscuro);
    LCD_Llenar_Rect(boss_x + 14, BOSS_Y + 17, 4, 8, color_oscuro);
    LCD_Llenar_Rect(boss_x + 15, BOSS_Y + 20, 2, 3, COLOR_BLANCO);
  }
  else if (indice == 1U)
  {
    LCD_Llenar_Rect(boss_x + 8, BOSS_Y, 16, 3, color_oscuro);
    LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 3, 24, 4, color_oscuro);
    LCD_Llenar_Rect(boss_x + 2, BOSS_Y + 7, 28, 25, color_oscuro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 32, 20, 4, color_oscuro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 4, 20, 4, color_claro);
    LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 8, 24, 22, color_claro);
    LCD_Llenar_Rect(boss_x + 8, BOSS_Y + 30, 16, 3, color_claro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 11, 5, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 13, 6, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 21, BOSS_Y + 11, 5, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 17, BOSS_Y + 13, 6, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 7, BOSS_Y + 17, 7, 5, COLOR_BLANCO);
    LCD_Llenar_Rect(boss_x + 18, BOSS_Y + 17, 7, 5, COLOR_BLANCO);
    LCD_Llenar_Rect(boss_x + 10, BOSS_Y + 18, 3, 4, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 19, BOSS_Y + 18, 3, 4, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 26, 14, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 7, BOSS_Y + 29, 4, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 21, BOSS_Y + 29, 4, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 12, BOSS_Y + 27, 3, 2, COLOR_BLANCO);
    LCD_Llenar_Rect(boss_x + 17, BOSS_Y + 27, 3, 2, COLOR_BLANCO);
  }
  else if (indice == 2U)
  {
    LCD_Llenar_Rect(boss_x + 1, BOSS_Y, 3, 36, COLOR_BLANCO);
    LCD_Llenar_Rect(boss_x, BOSS_Y, 5, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 4, 28, 28, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 5, BOSS_Y + 5, 26, 8, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 5, BOSS_Y + 13, 26, 9, 0xF800U);
    LCD_Llenar_Rect(boss_x + 5, BOSS_Y + 22, 26, 9, 0xFFE0U);
    LCD_Llenar_Rect(boss_x + 29, BOSS_Y + 5, 2, 3, fondo);
    LCD_Llenar_Rect(boss_x + 29, BOSS_Y + 28, 2, 3, fondo);
  }
  else
  {
    LCD_Llenar_Rect(boss_x + 10, BOSS_Y, 12, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 3, 20, 4, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 3, BOSS_Y + 7, 26, 5, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 1, BOSS_Y + 12, 30, 16, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 28, 24, 5, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 33, 14, 3, COLOR_NEGRO);
    LCD_Llenar_Rect(boss_x + 10, BOSS_Y + 3, 12, 4, color_claro);
    LCD_Llenar_Rect(boss_x + 6, BOSS_Y + 7, 20, 5, color_claro);
    LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 12, 23, 16, color_claro);
    LCD_Llenar_Rect(boss_x + 7, BOSS_Y + 28, 18, 4, color_claro);
    LCD_Llenar_Rect(boss_x + 10, BOSS_Y + 32, 12, 2, color_claro);
    LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 12, 4, 5, color_oscuro);
    LCD_Llenar_Rect(boss_x + 18, BOSS_Y + 12, 4, 5, color_oscuro);
    LCD_Llenar_Rect(boss_x + 10, BOSS_Y + 12, 2, 2, COLOR_BLANCO);
    LCD_Llenar_Rect(boss_x + 19, BOSS_Y + 12, 2, 2, COLOR_BLANCO);
    LCD_Llenar_Rect(boss_x + 24, BOSS_Y + 16, 8, 4, fondo);
    LCD_Llenar_Rect(boss_x + 20, BOSS_Y + 20, 12, 4, fondo);
    LCD_Llenar_Rect(boss_x + 24, BOSS_Y + 24, 8, 4, fondo);
  }
}

static void Game_DibujarVidaBoss(void)
{
  uint8_t indice = Game_IndiceBoss();
  uint8_t vida_maxima = Game_VidaMaximaBoss();
  int16_t ancho = (int16_t)((boss_vida * 94U) / vida_maxima);

  LCD_DibujarTexto(6, 9, nombres_boss[indice], 1U, COLOR_BLANCO,
                   colores_pantalla[pantalla_actual]);
  LCD_Llenar_Rect(58, 7, 98, 12, COLOR_NEGRO);
  LCD_Llenar_Rect(60, 9, 94, 8, colores_pantalla[pantalla_actual]);
  LCD_Llenar_Rect(60, 9, ancho, 8, colores_boss_claro[indice]);
}

static void Game_DibujarVidaJugador(void)
{
  uint16_t fondo = colores_pantalla[pantalla_actual];

  LCD_Llenar_Rect(180, 5, 130, 16, fondo);
  LCD_DibujarTexto(180, 8, "VIDA", 1U, COLOR_BLANCO, fondo);

  for (uint8_t i = 0U; i < JUGADOR_VIDA_MAX; i++)
  {
    int16_t x = (int16_t)(210 + (i * 15U));

    LCD_Llenar_Rect(x, 7, 12, 12, COLOR_NEGRO);
    LCD_Llenar_Rect(x + 2, 9, 8, 8,
                    (i < vida_jugador) ? COLOR_JUGADOR : fondo);
  }
}

static void Game_DibujarDisparo(void)
{
  if (disparo_activo)
  {
    LCD_Llenar_Rect(disparo_x, disparo_y, DISPARO_ANCHO,
                    DISPARO_ALTO, COLOR_BLANCO);
  }
}

static void Game_DibujarPistola(void)
{
  if ((pantalla_actual != 0U) || tiene_pistola)
  {
    return;
  }

  LCD_Llenar_Rect(PISTOLA_X, PISTOLA_Y, 14, 4, COLOR_BLANCO);
  LCD_Llenar_Rect(PISTOLA_X + 12, PISTOLA_Y + 1, 6, 3, COLOR_JUGADOR_DARK);
  LCD_Llenar_Rect(PISTOLA_X + 4, PISTOLA_Y + 4, 5, 5, COLOR_JUGADOR_DARK);
}

static void Game_BorrarDisparo(int16_t x, int16_t y)
{
  Game_RestaurarFondo(x, y, DISPARO_ANCHO, DISPARO_ALTO);

  if (((pantalla_actual == 0U) || Game_EsPantallaBoss()) &&
      ((y + DISPARO_ALTO) > PISO_Y))
  {
    LCD_Llenar_Rect(x, PISO_Y, DISPARO_ANCHO,
                    LCD_ALTO - PISO_Y, COLOR_PISO);
    LCD_Llenar_Rect(x, PISO_Y, DISPARO_ANCHO, 3, COLOR_PISO_TOP);
  }

  Game_DibujarPistola();
}

static void Game_ActualizarDisparo(bool disparar)
{
  if (disparo_activo)
  {
    Game_BorrarDisparo(disparo_x, disparo_y);
  }

  if (disparar)
  {
    Game_IniciarDisparo();
  }

  if (!disparo_activo)
  {
    return;
  }

  disparo_x = (int16_t)(disparo_x + disparo_dx);
  disparo_y = (int16_t)(disparo_y + disparo_dy);

  if ((disparo_x >= LCD_ANCHO) ||
      ((disparo_x + DISPARO_ANCHO) < 0) ||
      (disparo_y >= LCD_ALTO) ||
      ((disparo_y + DISPARO_ALTO) < 0) ||
      Game_DisparoTocaPlataforma())
  {
    disparo_activo = false;
    return;
  }

  Game_DibujarDisparo();
}

static bool Game_ActualizarBoss(bool disparar, bool *golpeado)
{
  int16_t boss_x_anterior = boss_x;
  int16_t disparo_x_anterior = disparo_x;
  int16_t disparo_y_anterior = disparo_y;
  bool disparo_anterior = disparo_activo;
  uint16_t fondo = colores_pantalla[pantalla_actual];

  *golpeado = false;

  LCD_Llenar_Rect(boss_x_anterior, BOSS_Y, BOSS_ANCHO, BOSS_ALTO, fondo);
  if (disparo_anterior)
  {
    LCD_Llenar_Rect(disparo_x_anterior, disparo_y_anterior,
                    DISPARO_ANCHO, DISPARO_ALTO, fondo);
  }

  boss_x = (int16_t)(boss_x + (boss_direccion * BOSS_SPEED));
  if (boss_x <= 12)
  {
    boss_x = 12;
    boss_direccion = 1;
  }
  else if (boss_x >= (LCD_ANCHO - BOSS_ANCHO - 12))
  {
    boss_x = LCD_ANCHO - BOSS_ANCHO - 12;
    boss_direccion = -1;
  }

  *golpeado = Game_ActualizarDisparosBoss();
  if (*golpeado)
  {
    return false;
  }

  if (disparar)
  {
    Game_IniciarDisparo();
  }

  if (disparo_activo)
  {
    disparo_x = (int16_t)(disparo_x + disparo_dx);
    disparo_y = (int16_t)(disparo_y + disparo_dy);

    if (((disparo_x + DISPARO_ANCHO) > boss_x) &&
        (disparo_x < (boss_x + BOSS_ANCHO)) &&
        ((disparo_y + DISPARO_ALTO) > BOSS_Y) &&
        (disparo_y < (BOSS_Y + BOSS_ALTO)))
    {
      disparo_activo = false;
      if (boss_vida > 0U)
      {
        boss_vida--;
      }

      if (boss_vida == 0U)
      {
        bosses_derrotados[Game_IndiceBoss()] = true;
        Game_LimpiarDisparosBoss();
        return true;
      }
    }
    else if ((disparo_y + DISPARO_ALTO) < 0)
    {
      disparo_activo = false;
    }
  }

  Game_DibujarBoss();
  Game_DibujarVidaBoss();
  Game_DibujarVidaJugador();
  Game_DibujarDisparo();
  return false;
}

static bool SD_Init(void)
{
  sd_lista = f_mount(&sd_fatfs, "0:", 1U) == FR_OK;
  return sd_lista;
}

static void SD_CerrarFondo(void)
{
  if (fondo_abierto)
  {
    f_close(&fondo_archivo);
    fondo_abierto = false;
    fondo_indice_abierto = 0xFFU;
  }
}

static bool SD_AbrirFondo(uint8_t indice)
{
  char nombre[] = "0:/FONDO01.BIN";

  if (!sd_lista || (indice >= TOTAL_FONDOS))
  {
    return false;
  }

  if (fondo_abierto && (fondo_indice_abierto == indice))
  {
    return true;
  }

  SD_CerrarFondo();
  indice++;
  nombre[8] = (char)('0' + (indice / 10U));
  nombre[9] = (char)('0' + (indice % 10U));

  if (f_open(&fondo_archivo, nombre, FA_READ) != FR_OK)
  {
    return false;
  }

  if (f_size(&fondo_archivo) != FONDO_TAMANO_BYTES)
  {
    f_close(&fondo_archivo);
    return false;
  }

  fondo_abierto = true;
  fondo_indice_abierto = (uint8_t)(indice - 1U);
  return true;
}

static bool SD_DibujarRegion(uint8_t indice, int16_t x, int16_t y,
                            int16_t ancho, int16_t alto)
{
  int16_t x0 = (x < 0) ? 0 : x;
  int16_t y0 = (y < 0) ? 0 : y;
  int16_t x1 = ((x + ancho) > LCD_ANCHO) ? LCD_ANCHO : (x + ancho);
  int16_t y1 = ((y + alto) > LCD_ALTO) ? LCD_ALTO : (y + alto);
  UINT bytes_leidos;
  UINT bytes_fila;

  if ((x0 >= x1) || (y0 >= y1) || !SD_AbrirFondo(indice))
  {
    return false;
  }

  bytes_fila = (UINT)((x1 - x0) * 2);
  LCD_SetAddressWindow((uint16_t)x0, (uint16_t)y0,
                       (uint16_t)(x1 - 1), (uint16_t)(y1 - 1));
  LCD_CS_LOW();
  LCD_RS_DATA();

  for (int16_t fila = y0; fila < y1; fila++)
  {
    FSIZE_t posicion = ((FSIZE_t)fila * LCD_ANCHO + (FSIZE_t)x0) * 2U;

    if ((f_lseek(&fondo_archivo, posicion) != FR_OK) ||
        (f_read(&fondo_archivo, fondo_buffer, bytes_fila,
                &bytes_leidos) != FR_OK) ||
        (bytes_leidos != bytes_fila))
    {
      LCD_CS_HIGH();
      SD_CerrarFondo();
      return false;
    }

    for (int16_t columna = 0; columna < (x1 - x0); columna++)
    {
      uint16_t color = fondo_buffer[columna];
      LCD_Write8((uint8_t)(color >> 8U));
      LCD_Write8((uint8_t)color);
    }
  }

  LCD_CS_HIGH();
  return true;
}

static uint8_t Game_IndiceFondo(void)
{
  return (uint8_t)(pantalla_actual - (pantalla_actual / 4U));
}

static void Game_RestaurarFondo(int16_t x, int16_t y, int16_t ancho, int16_t alto)
{
  if (Game_EsPantallaBoss() ||
      !SD_DibujarRegion(Game_IndiceFondo(), x, y, ancho, alto))
  {
    LCD_Llenar_Rect(x, y, ancho, alto, colores_pantalla[pantalla_actual]);
  }
}

static void Game_DibujarFondo(void)
{
  bool fondo_dibujado = false;

  if (Game_EsPantallaBoss())
  {
    SD_CerrarFondo();
  }
  else
  {
    fondo_dibujado = SD_DibujarRegion(Game_IndiceFondo(), 0, 0,
                                      LCD_ANCHO, LCD_ALTO);
  }

  if (!fondo_dibujado)
  {
    LCD_Llenar_Pantalla(colores_pantalla[pantalla_actual]);
  }

  if (Game_PlataformasActivas())
  {
    for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
    {
      Game_DibujarPlataforma(&plataformas[pantalla_actual][i]);
    }
  }

  if ((pantalla_actual == 0U) || Game_EsPantallaBoss())
  {
    LCD_Llenar_Rect(0, PISO_Y, LCD_ANCHO, LCD_ALTO - PISO_Y, COLOR_PISO);
    LCD_Llenar_Rect(0, PISO_Y, LCD_ANCHO, 3, COLOR_PISO_TOP);
  }

  Game_DibujarPistola();
  Game_DibujarBoss();
  if (Game_BossActivo())
  {
    Game_DibujarVidaBoss();
    Game_DibujarVidaJugador();
    Game_DibujarDisparo();
    Game_DibujarDisparosBoss();
  }
}

static const uint16_t *Game_SpriteJugador(void)
{
  if (salto_cargando)
  {
    return sprite_carga_derecha;
  }

  if (!jugador.on_ground)
  {
    return sprite_aire_derecha;
  }

  return sprite_idle_derecha;
}

static void LCD_DibujarSpriteJugador(int16_t x, int16_t y,
                                     const uint16_t *sprite, bool derecha)
{
  for (int16_t fila = 0; fila < JUGADOR_ALTO; fila++)
  {
    int16_t pantalla_y = y + fila;
    int16_t columna = 0;

    if ((pantalla_y < 0) || (pantalla_y >= LCD_ALTO))
    {
      continue;
    }

    while (columna < JUGADOR_ANCHO)
    {
      int16_t inicio;
      int16_t final;
      int16_t x0;
      int16_t x1;

      while (columna < JUGADOR_ANCHO)
      {
        int16_t origen = derecha ? columna : (JUGADOR_ANCHO - 1 - columna);
        uint16_t color = sprite[(fila * JUGADOR_ANCHO) + origen];

        if (color != COLOR_TRANSPARENTE)
        {
          break;
        }
        columna++;
      }

      if (columna >= JUGADOR_ANCHO)
      {
        break;
      }

      inicio = columna;
      while (columna < JUGADOR_ANCHO)
      {
        int16_t origen = derecha ? columna : (JUGADOR_ANCHO - 1 - columna);
        uint16_t color = sprite[(fila * JUGADOR_ANCHO) + origen];

        if (color == COLOR_TRANSPARENTE)
        {
          break;
        }
        columna++;
      }
      final = columna - 1;
      x0 = x + inicio;
      x1 = x + final;

      if ((x1 < 0) || (x0 >= LCD_ANCHO))
      {
        continue;
      }
      if (x0 < 0)
      {
        x0 = 0;
      }
      if (x1 >= LCD_ANCHO)
      {
        x1 = LCD_ANCHO - 1;
      }

      LCD_SetAddressWindow((uint16_t)x0, (uint16_t)pantalla_y,
                           (uint16_t)x1, (uint16_t)pantalla_y);
      LCD_CS_LOW();
      LCD_RS_DATA();

      for (int16_t pantalla_x = x0; pantalla_x <= x1; pantalla_x++)
      {
        int16_t destino = pantalla_x - x;
        int16_t origen = derecha ? destino : (JUGADOR_ANCHO - 1 - destino);
        uint16_t color = sprite[(fila * JUGADOR_ANCHO) + origen];

        LCD_Write8((uint8_t)(color >> 8U));
        LCD_Write8((uint8_t)color);
      }
      LCD_CS_HIGH();
    }
  }
}

static void Game_DibujarJugador(int16_t x, int16_t y)
{
  LCD_DibujarSpriteJugador(x, y, Game_SpriteJugador(), jugador_mira_derecha);
}

static void Game_BorrarJugador(int16_t x, int16_t y)
{
  Game_RestaurarFondo(x, y, JUGADOR_ANCHO, JUGADOR_ALTO);

  if (Game_PlataformasActivas())
  {
    for (uint8_t i = 0U; i < PLATAFORMAS_PANTALLA; i++)
    {
      const Plataforma_t *plataforma = &plataformas[pantalla_actual][i];

      if (((x + JUGADOR_ANCHO) > plataforma->x) &&
          (x < (plataforma->x + plataforma->ancho)) &&
          ((y + JUGADOR_ALTO) > plataforma->y) &&
          (y < (plataforma->y + plataforma->alto)))
      {
        Game_DibujarPlataforma(plataforma);
      }
    }
  }

  if (((pantalla_actual == 0U) || Game_EsPantallaBoss()) &&
      ((y + JUGADOR_ALTO) > PISO_Y))
  {
    LCD_Llenar_Rect(x, PISO_Y, JUGADOR_ANCHO,
                    LCD_ALTO - PISO_Y, COLOR_PISO);
    LCD_Llenar_Rect(x, PISO_Y, JUGADOR_ANCHO, 3, COLOR_PISO_TOP);
  }

  Game_DibujarPistola();

}

static void Game_Iniciar(uint8_t pantalla_inicial, bool pistola_inicial)
{
  Audio_Detener();
  pantalla_actual = (pantalla_inicial < TOTAL_PANTALLAS) ?
                    pantalla_inicial : 0U;
  jugador.x = (LCD_ANCHO - JUGADOR_ANCHO) / 2;
  jugador.vx = 0;
  jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
  jugador.vy_fp = 0L;
  jugador.on_ground = true;
  jump_apachado = false;
  salto_cargando = false;
  salto_inicio = 0U;
  salto_direccion = 0;
  jugador_frame = 0U;
  jugador_mira_derecha = true;
  proximo_frame_caminata = HAL_GetTick() + WALK_FRAME_MS;
  memset(bosses_derrotados, 0, sizeof(bosses_derrotados));
  for (uint8_t i = 0U; i < (pantalla_actual / 4U); i++)
  {
    bosses_derrotados[i] = true;
  }
  boss_vida = 0U;
  disparo_activo = false;
  disparo_dx = 0;
  disparo_dy = 0;
  tiene_pistola = pistola_inicial;
  shoot_anterior = false;
  pause_anterior = Control_Presionado(Control_Leer(), CONTROL_PAUSA);
  Game_LimpiarDisparosBoss();
  proximo_disparo_boss = 0U;
  patron_disparo_boss = 0U;
  vida_jugador = 0U;
  salto_en_curso = false;

  if (Game_BossActivo())
  {
    Game_BossInit();
  }

  Game_DibujarFondo();
  Game_DibujarJugador(jugador.x, (int16_t)(jugador.y_fp >> FP_SHIFT));
}

static void Game_Init(void)
{
  Game_Iniciar(0U, false);
}

static void Game_InitBoss(uint8_t indice)
{
  if (indice >= TOTAL_BOSSES)
  {
    indice = 0U;
  }

  Game_Iniciar((uint8_t)((indice * 4U) + 3U), true);
}

static void Game_Update(void)
{
  uint8_t control = Control_Leer();
  bool left_apachado = Control_Presionado(control, CONTROL_IZQUIERDA);
  bool right_apachado = Control_Presionado(control, CONTROL_DERECHA);
  bool jump_pressed = Control_Presionado(control, CONTROL_SALTO);
  bool shoot_pressed = Control_Presionado(control, CONTROL_DISPARO);
  bool pause_pressed = Control_Presionado(control, CONTROL_PAUSA);
  bool disparar = shoot_pressed && !shoot_anterior && tiene_pistola;
  int16_t old_x = jugador.x;
  int16_t old_y = (int16_t)(jugador.y_fp >> FP_SHIFT);
  uint8_t old_frame = jugador_frame;
  bool old_direccion = jugador_mira_derecha;
  bool old_salto_cargando = salto_cargando;
  bool cambio_pantalla = false;
  bool colision_pared = false;
  bool estaba_en_suelo = jugador.on_ground;
  int16_t movimiento_x = 0;

  if (pause_pressed && !pause_anterior)
  {
    Pausa_Init();
    return;
  }

  pause_anterior = pause_pressed;
  shoot_anterior = shoot_pressed;

  if (Game_BossActivo())
  {
    jugador.vx = 0;

    if ((!jump_pressed || !jugador.on_ground) &&
        left_apachado && !right_apachado)
    {
      movimiento_x = -PLAYER_SPEED;
      jugador_mira_derecha = false;
    }
    else if ((!jump_pressed || !jugador.on_ground) &&
             right_apachado && !left_apachado)
    {
      movimiento_x = PLAYER_SPEED;
      jugador_mira_derecha = true;
    }
  }
  else if (jugador.on_ground)
  {
    jugador.vx = 0;

    if (left_apachado && !right_apachado)
    {
      jugador_mira_derecha = false;
    }
    else if (right_apachado && !left_apachado)
    {
      jugador_mira_derecha = true;
    }
  }
  else
  {
    movimiento_x = jugador.vx;
  }

  int16_t destino_x = old_x + movimiento_x;

  if (destino_x < 0)
  {
    destino_x = 0;
    colision_pared = true;
  }
  else if (destino_x > (LCD_ANCHO - JUGADOR_ANCHO))
  {
    destino_x = LCD_ANCHO - JUGADOR_ANCHO;
    colision_pared = true;
  }

  jugador.x = Game_ResolverHorizontal(old_x, destino_x, old_y);
  if (colision_pared)
  {
    if (!Game_BossActivo() && !jugador.on_ground)
    {
      int16_t fuerza = (jugador.vx < 0) ? -jugador.vx : jugador.vx;
      int16_t rebote = (fuerza + 1) / 2;

      if (destino_x == 0)
      {
        jugador.vx = rebote;
        jugador_mira_derecha = true;
      }
      else
      {
        jugador.vx = -rebote;
        jugador_mira_derecha = false;
      }
    }
    else
    {
      jugador.vx = 0;
    }
  }

  bool caminando = jugador.x != old_x;
  uint32_t ahora = HAL_GetTick();

  if (caminando)
  {
    if ((int32_t)(ahora - proximo_frame_caminata) >= 0)
    {
      jugador_frame = (uint8_t)((jugador_frame + 1U) % WALK_FRAMES);
      proximo_frame_caminata = ahora + WALK_FRAME_MS;
    }
  }
  else
  {
    jugador_frame = 0U;
    proximo_frame_caminata = ahora + WALK_FRAME_MS;
  }

  if (jugador.on_ground && !Game_TieneSoporte(jugador.x, old_y))
  {
    jugador.on_ground = false;
    salto_cargando = false;
  }

  //Iniciar carga del salto
  if (jump_pressed && !jump_apachado && jugador.on_ground)
  {
    salto_cargando = true;
    salto_inicio = ahora;
    salto_direccion = 0;

    if (left_apachado && !right_apachado)
    {
      salto_direccion = -1;
    }
    else if (right_apachado && !left_apachado)
    {
      salto_direccion = 1;
    }
  }

  if (salto_cargando)
  {
    uint32_t carga = ahora - salto_inicio;
    bool soltar = !jump_pressed && jump_apachado;
    bool carga_maxima = carga >= JUMP_CHARGE_MS;

    if (left_apachado && !right_apachado)
    {
      salto_direccion = -1;
      jugador_mira_derecha = false;
    }
    else if (right_apachado && !left_apachado)
    {
      salto_direccion = 1;
      jugador_mira_derecha = true;
    }

    if (soltar || carga_maxima)
    {
      if (carga > JUMP_CHARGE_MS)
      {
        carga = JUMP_CHARGE_MS;
      }

      jugador.vy_fp = JUMP_MIN_SPEED_FP +
                       (((JUMP_MAX_SPEED_FP - JUMP_MIN_SPEED_FP) *
                         (int32_t)carga) / (int32_t)JUMP_CHARGE_MS);

      if (Game_BossActivo())
      {
        jugador.vx = 0;
      }
      else
      {
        int16_t velocidad_x = JUMP_MIN_SPEED_X +
                              (int16_t)(((JUMP_MAX_SPEED_X - JUMP_MIN_SPEED_X) *
                                         carga) / JUMP_CHARGE_MS);
        jugador.vx = (int16_t)(salto_direccion * velocidad_x);
      }

      jugador.on_ground = false;
      salto_cargando = false;
      salto_en_curso = true;
    }
  }

  jump_apachado = jump_pressed;

  if (!jugador.on_ground)
  {
    jugador.vy_fp += GRAVEDAD_FP;
    if (jugador.vy_fp > MAX_FALL_SPEED_FP)
    {
      jugador.vy_fp = MAX_FALL_SPEED_FP;
    }

    int32_t velocidad_y = jugador.vy_fp;
    int32_t destino_y_fp = jugador.y_fp + jugador.vy_fp;
    int16_t destino_y = (int16_t)(destino_y_fp >> FP_SHIFT);
    int16_t posicion_y = Game_ResolverVertical(old_y, destino_y);

    if ((jugador.vy_fp == velocidad_y) && !jugador.on_ground)
    {
      jugador.y_fp = destino_y_fp;
    }
    else
    {
      jugador.y_fp = (int32_t)posicion_y * FP_UNO;
    }
  }

  if (!estaba_en_suelo && jugador.on_ground && salto_en_curso)
  {
    Audio_EfectoAterrizaje();
    salto_en_curso = false;
  }

  int16_t new_y = (int16_t)(jugador.y_fp >> FP_SHIFT);

  if (Game_JugadorTocaPistola(jugador.x, new_y))
  {
    tiene_pistola = true;
    disparo_activo = false;
    Game_DibujarFondo();
    Game_DibujarJugador(jugador.x, new_y);
    return;
  }

  if (Game_BossActivo())
  {
    bool golpeado = false;

    if (Game_ActualizarBoss(disparar, &golpeado))
    {
      Game_DibujarFondo();
      Game_DibujarJugador(jugador.x, new_y);
      return;
    }

    if (golpeado)
    {
      if (vida_jugador > 0U)
      {
        vida_jugador--;
      }

      if (vida_jugador == 0U)
      {
        estado_actual = ESTADO_JUEGO;
        Game_Init();
        return;
      }

      jugador.x = (LCD_ANCHO - JUGADOR_ANCHO) / 2;
      jugador.vx = 0;
      jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
      jugador.vy_fp = 0L;
      jugador.on_ground = true;
      salto_cargando = false;
      salto_en_curso = false;
      disparo_activo = false;
      Game_LimpiarDisparosBoss();
      proximo_disparo_boss = HAL_GetTick() + 500U;
      Game_DibujarFondo();
      Game_DibujarJugador(jugador.x, PISO_Y - JUGADOR_ALTO);
      return;
    }
  }

  if ((new_y + JUGADOR_ALTO) < 0)
  {
    if (pantalla_actual < (TOTAL_PANTALLAS - 1U))
    {
      pantalla_actual++;
      if (Game_BossActivo())
      {
        Game_BossInit();
        jugador.vx = 0;
        jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
        jugador.vy_fp = 0L;
        jugador.on_ground = true;
        salto_en_curso = false;
      }
      else
      {
        jugador.y_fp = (int32_t)(LCD_ALTO - 1) * FP_UNO;
        jugador.on_ground = false;
      }
      cambio_pantalla = true;
    }
    else
    {
      jugador.y_fp = 0L;
      jugador.vy_fp = 0L;
      jugador.on_ground = false;
    }
  }
  else if (new_y >= LCD_ALTO)
  {
    if (pantalla_actual > 0U)
    {
      pantalla_actual--;
      jugador.y_fp = (int32_t)(1 - JUGADOR_ALTO) * FP_UNO;
      jugador.on_ground = false;
      cambio_pantalla = true;
    }
    else
    {
      jugador.vx = 0;
      jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
      jugador.vy_fp = 0L;
      jugador.on_ground = true;
      salto_en_curso = false;
    }
  }

  new_y = (int16_t)(jugador.y_fp >> FP_SHIFT);

  if (cambio_pantalla)
  {
    disparo_activo = false;
    Game_DibujarFondo();
    Game_DibujarJugador(jugador.x, new_y);
    return;
  }

  if ((jugador.x != old_x) || (new_y != old_y) ||
      (jugador_frame != old_frame) ||
      (jugador_mira_derecha != old_direccion) ||
      (salto_cargando != old_salto_cargando) ||
      (jugador.on_ground != estaba_en_suelo))
  {
    Game_BorrarJugador(old_x, old_y);
    Game_DibujarJugador(jugador.x, new_y);
  }

  if (!Game_BossActivo())
  {
    Game_ActualizarDisparo(disparar);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint32_t next_frame;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM10_Init();
  MX_UART4_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  Audio_Init();
  Control_Iniciar();

  //Deseleccionar los dispositivos SPI
  HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  SD_Init();

  LCD_HardwareReset();

  LCD_InitPantalla();
  Menu_Init();

  next_frame = HAL_GetTick() + FRAME_TIME_MS;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    Audio_Update();

    if ((int32_t)(now - next_frame) >= 0)
    {
      if (estado_actual == ESTADO_MENU)
      {
        Menu_Update();
      }
      else if (estado_actual == ESTADO_LOAD)
      {
        Load_Update();
      }
      else if (estado_actual == ESTADO_JUEGO)
      {
        Game_Update();
      }
      else if (estado_actual == ESTADO_PAUSA)
      {
        Pausa_Update();
      }
      else
      {
        Dormido_Update();
      }

      next_frame += FRAME_TIME_MS;

      //Evitar tener frames atrasados
      if ((int32_t)(now - next_frame) > (int32_t)(FRAME_TIME_MS * 4U))
      {
        next_frame = now + FRAME_TIME_MS;
      }
    }

    HAL_Delay(1U);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 179;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 999;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim10, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */
  HAL_TIM_MspPostInit(&htim10);

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, F_CS_Pin|LCD_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RS_Pin|LCD_D7_Pin|LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin|LCD_D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : F_CS_Pin LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = F_CS_Pin|LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin SD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == UART4)
  {
    if ((control_rx_byte & CONTROL_VALIDO) != 0U)
    {
      control_estado = control_rx_byte &
                       (CONTROL_IZQUIERDA | CONTROL_DERECHA |
                        CONTROL_SALTO | CONTROL_DISPARO | CONTROL_PAUSA);
      control_ultimo_ms = HAL_GetTick();
    }

    HAL_UART_Receive_IT(&huart4, &control_rx_byte, 1U);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == UART4)
  {
    control_estado = 0U;
    HAL_UART_Receive_IT(&huart4, &control_rx_byte, 1U);
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
