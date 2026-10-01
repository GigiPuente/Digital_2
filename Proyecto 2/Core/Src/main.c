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

typedef enum
{
  ESTADO_MENU = 0,
  ESTADO_JUEGO,
  ESTADO_DORMIDO
} Estado_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_ANCHO               320
#define LCD_ALTO                240

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

#define JUGADOR_ANCHO            16
#define JUGADOR_ALTO             20
#define WALK_FRAMES               2U
#define PLAYER_SPEED             3
#define WALK_FRAME_MS            120U
#define PISO_Y                   210
#define TOTAL_PANTALLAS           12U
#define PLATAFORMAS_PANTALLA       5U
#define BOSS_ANCHO                32
#define BOSS_ALTO                 36
#define BOSS_Y                    45
#define BOSS_SPEED                 1
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
#define JUMP_SPEED_FP             (-1792L)
#define MAX_FALL_SPEED_FP         2048L
#define FRAME_TIME_MS             16U

#define MENU_EMPEZAR               0U
#define MENU_SALIR                 1U

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
/* USER CODE BEGIN PV */
static Jugador_t jugador;
static bool jump_apachado = false;
static Estado_t estado_actual = ESTADO_MENU;
static uint8_t opcion_menu = MENU_EMPEZAR;
static bool left_anterior = false;
static bool right_anterior = false;
static bool jump_anterior = false;
static bool shoot_anterior = false;
static bool despertar_armado = false;
static uint8_t jugador_frame = 0U;
static bool jugador_mira_derecha = true;
static uint32_t proximo_frame_caminata = 0U;
static uint8_t pantalla_actual = 0U;
static bool bosses_derrotados[3] = {false, false, false};
static int16_t boss_x = 0;
static int8_t boss_direccion = 1;
static uint8_t boss_vida = 0U;
static bool disparo_activo = false;
static int16_t disparo_x = 0;
static int16_t disparo_y = 0;
static int16_t disparo_dx = 0;
static int16_t disparo_dy = 0;
static bool tiene_pistola = false;
static const uint16_t colores_pantalla[TOTAL_PANTALLAS] = {
  0x10A6U, 0x2149U, 0x3128U, 0x1803U,
  0x0328U, 0x03CBU, 0x046DU, 0x0183U,
  0x4008U, 0x600CU, 0x8010U, 0x2004U
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
/* USER CODE BEGIN PFP */
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

static bool Boton_Apachado(GPIO_TypeDef *port, uint16_t pin);
static void Botones_Sincronizar(void);
static void Menu_Dibujar(void);
static void Menu_Init(void);
static void Menu_Update(void);
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
static bool Game_ActualizarBoss(bool disparar);
static bool Game_JugadorTocaPistola(int16_t x, int16_t y);
static bool Game_DisparoTocaPlataforma(void);
static void Game_IniciarDisparo(void);
static void Game_ActualizarDisparo(bool disparar);
static void Game_DibujarPlataforma(const Plataforma_t *plataforma);
static void Game_DibujarBoss(void);
static void Game_DibujarVidaBoss(void);
static void Game_DibujarDisparo(void);
static void Game_DibujarPistola(void);
static void Game_BorrarDisparo(int16_t x, int16_t y);
static void Game_DibujarFondo(void);
static void Game_DibujarJugador(int16_t x, int16_t y);
static void Game_BorrarJugador(int16_t x, int16_t y);
static void Game_Init(void);
static void Game_Update(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

static bool Boton_Apachado(GPIO_TypeDef *port, uint16_t pin)
{
  return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

static void Botones_Sincronizar(void)
{
  left_anterior = Boton_Apachado(BTN_LEFT_GPIO_Port, BTN_LEFT_Pin);
  right_anterior = Boton_Apachado(BTN_RIGHT_GPIO_Port, BTN_RIGHT_Pin);
  jump_anterior = Boton_Apachado(BTN_JUMP_GPIO_Port, BTN_JUMP_Pin);
  shoot_anterior = Boton_Apachado(BTN_SHOOT_GPIO_Port, BTN_SHOOT_Pin);
}

static void Menu_Dibujar(void)
{
  const char titulo[] = "JUMP KING";
  const char empezar[] = "EMPEZAR";
  const char salir[] = "SALIR";
  uint16_t color_empezar = (opcion_menu == MENU_EMPEZAR) ? COLOR_SELECCION : COLOR_OPCION;
  uint16_t color_salir = (opcion_menu == MENU_SALIR) ? COLOR_SELECCION : COLOR_OPCION;
  int16_t titulo_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(titulo, 4U)) / 2U);
  int16_t empezar_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(empezar, 3U)) / 2U);
  int16_t salir_x = (int16_t)((LCD_ANCHO - LCD_AnchoTexto(salir, 3U)) / 2U);

  LCD_Llenar_Pantalla(COLOR_MENU);
  LCD_DibujarTexto(titulo_x, 18, titulo, 4U, COLOR_TITULO, COLOR_MENU);
  LCD_Llenar_Rect(45, 58, 230, 3, COLOR_TITULO);

  LCD_Llenar_Rect(65, 92, 190, 39, color_empezar);
  LCD_DibujarTexto(empezar_x, 101, empezar, 3U,
                   (opcion_menu == MENU_EMPEZAR) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_empezar);

  LCD_Llenar_Rect(65, 148, 190, 39, color_salir);
  LCD_DibujarTexto(salir_x, 157, salir, 3U,
                   (opcion_menu == MENU_SALIR) ? COLOR_NEGRO : COLOR_BLANCO,
                   color_salir);
}

static void Menu_Init(void)
{
  estado_actual = ESTADO_MENU;
  opcion_menu = MENU_EMPEZAR;
  despertar_armado = false;
  Menu_Dibujar();
  Botones_Sincronizar();
}

static void Menu_Update(void)
{
  bool left_actual = Boton_Apachado(BTN_LEFT_GPIO_Port, BTN_LEFT_Pin);
  bool right_actual = Boton_Apachado(BTN_RIGHT_GPIO_Port, BTN_RIGHT_Pin);
  bool jump_actual = Boton_Apachado(BTN_JUMP_GPIO_Port, BTN_JUMP_Pin);
  bool mover_opcion = (left_actual && !left_anterior) ||
                      (right_actual && !right_anterior);
  bool confirmar = jump_actual && !jump_anterior;

  left_anterior = left_actual;
  right_anterior = right_actual;
  jump_anterior = jump_actual;

  if (mover_opcion)
  {
    opcion_menu = (opcion_menu == MENU_EMPEZAR) ? MENU_SALIR : MENU_EMPEZAR;
    Menu_Dibujar();
  }

  if (confirmar)
  {
    if (opcion_menu == MENU_EMPEZAR)
    {
      estado_actual = ESTADO_JUEGO;
      Game_Init();
      jump_apachado = jump_actual;
    }
    else
    {
      LCD_Apagar();
      estado_actual = ESTADO_DORMIDO;
      despertar_armado = false;
    }
  }
}

static void Dormido_Update(void)
{
  bool algun_boton = Boton_Apachado(BTN_LEFT_GPIO_Port, BTN_LEFT_Pin) ||
                     Boton_Apachado(BTN_RIGHT_GPIO_Port, BTN_RIGHT_Pin) ||
                     Boton_Apachado(BTN_JUMP_GPIO_Port, BTN_JUMP_Pin) ||
                     Boton_Apachado(BTN_SHOOT_GPIO_Port, BTN_SHOOT_Pin);

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
  disparo_activo = false;
  disparo_dx = 0;
  disparo_dy = 0;
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
  uint16_t color_claro;
  uint16_t color_oscuro;

  if (!Game_BossActivo())
  {
    return;
  }

  if (Game_IndiceBoss() == 0U)
  {
    color_claro = 0xF800U;
    color_oscuro = 0x7800U;
  }
  else if (Game_IndiceBoss() == 1U)
  {
    color_claro = 0x07E0U;
    color_oscuro = 0x03E0U;
  }
  else
  {
    color_claro = COLOR_BOSS_CLARO;
    color_oscuro = COLOR_BOSS_OSCURO;
  }

  LCD_Llenar_Rect(boss_x + 2, BOSS_Y, 6, 8, color_oscuro);
  LCD_Llenar_Rect(boss_x + 24, BOSS_Y, 6, 8, color_oscuro);
  LCD_Llenar_Rect(boss_x, BOSS_Y + 6, 32, 26, color_claro);
  LCD_Llenar_Rect(boss_x + 4, BOSS_Y + 32, 8, 4, color_oscuro);
  LCD_Llenar_Rect(boss_x + 20, BOSS_Y + 32, 8, 4, color_oscuro);
  LCD_Llenar_Rect(boss_x + 7, BOSS_Y + 13, 5, 5, COLOR_BOSS_OJOS);
  LCD_Llenar_Rect(boss_x + 20, BOSS_Y + 13, 5, 5, COLOR_BOSS_OJOS);
  LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 15, 2, 2, COLOR_NEGRO);
  LCD_Llenar_Rect(boss_x + 21, BOSS_Y + 15, 2, 2, COLOR_NEGRO);
  LCD_Llenar_Rect(boss_x + 9, BOSS_Y + 24, 14, 3, color_oscuro);
}

static void Game_DibujarVidaBoss(void)
{
  uint8_t vida_maxima = Game_VidaMaximaBoss();
  int16_t ancho = (int16_t)((boss_vida * 84U) / vida_maxima);

  LCD_DibujarTexto(6, 6, "BOSS", 2U, COLOR_BOSS_OJOS,
                   colores_pantalla[pantalla_actual]);
  LCD_Llenar_Rect(70, 7, 88, 12, COLOR_NEGRO);
  LCD_Llenar_Rect(72, 9, 84, 8, colores_pantalla[pantalla_actual]);
  LCD_Llenar_Rect(72, 9, ancho, 8, COLOR_BOSS_OJOS);
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
  LCD_Llenar_Rect(x, y, DISPARO_ANCHO, DISPARO_ALTO,
                  colores_pantalla[pantalla_actual]);

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

static bool Game_ActualizarBoss(bool disparar)
{
  int16_t boss_x_anterior = boss_x;
  int16_t disparo_x_anterior = disparo_x;
  int16_t disparo_y_anterior = disparo_y;
  bool disparo_anterior = disparo_activo;
  uint16_t fondo = colores_pantalla[pantalla_actual];

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
  Game_DibujarDisparo();
  return false;
}

static void Game_DibujarFondo(void)
{
  LCD_Llenar_Pantalla(colores_pantalla[pantalla_actual]);

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
    Game_DibujarDisparo();
  }
}

static void Game_DibujarJugador(int16_t x, int16_t y)
{
  int16_t ojo_x = jugador_mira_derecha ? 9 : 6;

  LCD_Llenar_Rect(x + 5, y, 6, 6, COLOR_JUGADOR);
  LCD_Llenar_Rect(x + ojo_x, y + 2, 1, 1, COLOR_NEGRO);
  LCD_Llenar_Rect(x + 4, y + 6, 8, 8, COLOR_BLANCO);
  LCD_Llenar_Rect(x + 2, y + 7, 2, 7, COLOR_JUGADOR);
  LCD_Llenar_Rect(x + 12, y + 7, 2, 7, COLOR_JUGADOR);
  LCD_Llenar_Rect(x + 4, y + 13, 8, 2, COLOR_NEGRO);

  if (jugador_frame == 0U)
  {
    LCD_Llenar_Rect(x + 4, y + 15, 3, 5, COLOR_JUGADOR_DARK);
    LCD_Llenar_Rect(x + 9, y + 15, 3, 5, COLOR_JUGADOR_DARK);
  }
  else
  {
    LCD_Llenar_Rect(x + 2, y + 15, 4, 5, COLOR_JUGADOR_DARK);
    LCD_Llenar_Rect(x + 10, y + 15, 4, 5, COLOR_JUGADOR_DARK);
  }
}

static void Game_BorrarJugador(int16_t x, int16_t y)
{
  LCD_Llenar_Rect(x, y, JUGADOR_ANCHO, JUGADOR_ALTO,
                  colores_pantalla[pantalla_actual]);

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

static void Game_Init(void)
{
  pantalla_actual = 0U;
  jugador.x = (LCD_ANCHO - JUGADOR_ANCHO) / 2;
  jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
  jugador.vy_fp = 0L;
  jugador.on_ground = true;
  jump_apachado = false;
  jugador_frame = 0U;
  jugador_mira_derecha = true;
  proximo_frame_caminata = HAL_GetTick() + WALK_FRAME_MS;
  memset(bosses_derrotados, 0, sizeof(bosses_derrotados));
  boss_vida = 0U;
  disparo_activo = false;
  disparo_dx = 0;
  disparo_dy = 0;
  tiene_pistola = false;
  shoot_anterior = false;

  Game_DibujarFondo();
  Game_DibujarJugador(jugador.x, (int16_t)(jugador.y_fp >> FP_SHIFT));
}

static void Game_Update(void)
{
  bool left_apachado = Boton_Apachado(BTN_LEFT_GPIO_Port, BTN_LEFT_Pin);
  bool right_apachado = Boton_Apachado(BTN_RIGHT_GPIO_Port, BTN_RIGHT_Pin);
  bool jump_pressed = Boton_Apachado(BTN_JUMP_GPIO_Port, BTN_JUMP_Pin);
  bool shoot_pressed = Boton_Apachado(BTN_SHOOT_GPIO_Port, BTN_SHOOT_Pin);
  bool disparar = shoot_pressed && !shoot_anterior && tiene_pistola;
  int16_t old_x = jugador.x;
  int16_t old_y = (int16_t)(jugador.y_fp >> FP_SHIFT);
  uint8_t old_frame = jugador_frame;
  bool old_direccion = jugador_mira_derecha;
  bool cambio_pantalla = false;
  int16_t movimiento_x = 0;

  shoot_anterior = shoot_pressed;

  if (left_apachado && !right_apachado)
  {
    movimiento_x = -PLAYER_SPEED;
    jugador_mira_derecha = false;
  }
  else if (right_apachado && !left_apachado)
  {
    movimiento_x = PLAYER_SPEED;
    jugador_mira_derecha = true;
  }

  int16_t destino_x = old_x + movimiento_x;

  if (destino_x < 0)
  {
    destino_x = 0;
  }
  else if (destino_x > (LCD_ANCHO - JUGADOR_ANCHO))
  {
    destino_x = LCD_ANCHO - JUGADOR_ANCHO;
  }

  jugador.x = Game_ResolverHorizontal(old_x, destino_x, old_y);

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

  if (Game_BossActivo())
  {
    jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
    jugador.vy_fp = 0L;
    jugador.on_ground = true;
    jump_apachado = jump_pressed;

    if (Game_ActualizarBoss(disparar))
    {
      Game_DibujarFondo();
      Game_DibujarJugador(jugador.x, PISO_Y - JUGADOR_ALTO);
      return;
    }

    if ((jugador.x != old_x) ||
        (jugador_frame != old_frame) ||
        (jugador_mira_derecha != old_direccion))
    {
      Game_BorrarJugador(old_x, old_y);
      Game_DibujarJugador(jugador.x, PISO_Y - JUGADOR_ALTO);
    }
    return;
  }

  if (jugador.on_ground && !Game_TieneSoporte(jugador.x, old_y))
  {
    jugador.on_ground = false;
  }

  //Iniciar salto cuando se apache
  if (jump_pressed && !jump_apachado && jugador.on_ground)
  {
    jugador.vy_fp = JUMP_SPEED_FP;
    jugador.on_ground = false;
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

  int16_t new_y = (int16_t)(jugador.y_fp >> FP_SHIFT);

  if (Game_JugadorTocaPistola(jugador.x, new_y))
  {
    tiene_pistola = true;
    disparo_activo = false;
    Game_DibujarFondo();
    Game_DibujarJugador(jugador.x, new_y);
    return;
  }

  if ((new_y + JUGADOR_ALTO) < 0)
  {
    if (pantalla_actual < (TOTAL_PANTALLAS - 1U))
    {
      pantalla_actual++;
      if (Game_BossActivo())
      {
        Game_BossInit();
        jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
        jugador.vy_fp = 0L;
        jugador.on_ground = true;
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
      jugador.y_fp = (int32_t)(PISO_Y - JUGADOR_ALTO) * FP_UNO;
      jugador.vy_fp = 0L;
      jugador.on_ground = true;
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
      (jugador_mira_derecha != old_direccion))
  {
    Game_BorrarJugador(old_x, old_y);
    Game_DibujarJugador(jugador.x, new_y);
  }

  Game_ActualizarDisparo(disparar);
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

  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  //Deseleccionar los dispositivos SPI
  HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

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

    if ((int32_t)(now - next_frame) >= 0)
    {
      if (estado_actual == ESTADO_MENU)
      {
        Menu_Update();
      }
      else if (estado_actual == ESTADO_JUEGO)
      {
        Game_Update();
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

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, F_CS_Pin | LCD_RST_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin | LCD_WR_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOA, LCD_RS_Pin | LD2_Pin | LCD_D7_Pin | LCD_D0_Pin
                         | LCD_D2_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin | SD_CS_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOB, LCD_D6_Pin | LCD_D3_Pin | LCD_D5_Pin | LCD_D4_Pin,
                    GPIO_PIN_RESET);

  HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = F_CS_Pin | LCD_RST_Pin | LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_RD_Pin | LCD_WR_Pin | LCD_RS_Pin | LCD_D7_Pin
                        | LCD_D0_Pin | LCD_D2_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_CS_Pin | LCD_D6_Pin | LCD_D3_Pin | LCD_D5_Pin
                        | LCD_D4_Pin | SD_CS_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BTN_LEFT_Pin | BTN_RIGHT_Pin | BTN_JUMP_Pin
                        | BTN_SHOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */
