/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Prueba de LCD y movimiento tipo Jump King
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
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
} Player_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_WIDTH                 320
#define LCD_HEIGHT                240

#define COLOR_BLACK               0x0000U
#define COLOR_WHITE               0xFFFFU
#define COLOR_SKY                 0x10A6U
#define COLOR_GROUND              0x5AEBU
#define COLOR_GROUND_TOP          0xC618U
#define COLOR_PLAYER              0xFD20U
#define COLOR_PLAYER_DARK         0xA260U

#define PLAYER_WIDTH              16
#define PLAYER_HEIGHT             20
#define PLAYER_SPEED              3
#define GROUND_Y                  210

#define FP_SHIFT                  8
#define FP_ONE                    (1L << FP_SHIFT)
#define GRAVITY_FP                80L
#define JUMP_SPEED_FP             (-1792L)
#define MAX_FALL_SPEED_FP         2048L
#define FRAME_TIME_MS             16U

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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static Player_t player;
static bool jump_was_pressed = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void UART_Log(const char *text);

static inline void LCD_Write8(uint8_t value);
static uint8_t LCD_Read8(void);
static void LCD_SetDataBusMode(uint32_t mode);
static void LCD_HardwareReset(void);
static uint16_t LCD_ReadID(void);
static void LCD_WriteCommand(uint8_t command);
static void LCD_WriteCommandData(uint8_t command, const uint8_t *data, uint32_t length);
static void LCD_InitILI9341(void);
static void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
static void LCD_FillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color);
static void LCD_FillScreen(uint16_t color);

static bool ButtonPressed(GPIO_TypeDef *port, uint16_t pin);
static void Game_DrawScene(void);
static void Game_DrawPlayer(int16_t x, int16_t y);
static void Game_ErasePlayer(int16_t x, int16_t y);
static void Game_Init(void);
static void Game_Update(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void UART_Log(const char *text)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)text, (uint16_t)strlen(text), HAL_MAX_DELAY);
}

/*
 * El bus de datos del shield esta repartido entre GPIOA, GPIOB y GPIOC.
 * BSRR permite actualizar cada puerto sin ocho llamadas HAL por cada byte.
 */
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

static uint8_t LCD_Read8(void)
{
  uint8_t value = 0U;

  LCD_RD_LOW();
  for (volatile uint32_t delay = 0U; delay < 12U; delay++)
  {
    __NOP();
  }

  if ((GPIOA->IDR & LCD_D0_Pin) != 0U) value |= 0x01U;
  if ((GPIOC->IDR & LCD_D1_Pin) != 0U) value |= 0x02U;
  if ((GPIOA->IDR & LCD_D2_Pin) != 0U) value |= 0x04U;
  if ((GPIOB->IDR & LCD_D3_Pin) != 0U) value |= 0x08U;
  if ((GPIOB->IDR & LCD_D4_Pin) != 0U) value |= 0x10U;
  if ((GPIOB->IDR & LCD_D5_Pin) != 0U) value |= 0x20U;
  if ((GPIOB->IDR & LCD_D6_Pin) != 0U) value |= 0x40U;
  if ((GPIOA->IDR & LCD_D7_Pin) != 0U) value |= 0x80U;

  LCD_RD_HIGH();
  return value;
}

static void LCD_SetDataBusMode(uint32_t mode)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Mode = mode;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  GPIO_InitStruct.Pin = LCD_DATA_A_MASK;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_DATA_B_MASK;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_DATA_C_MASK;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
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

static uint16_t LCD_ReadID(void)
{
  uint8_t id_bytes[4];

  LCD_CS_LOW();
  LCD_RS_COMMAND();
  LCD_Write8(0xD3U);
  LCD_RS_DATA();

  LCD_SetDataBusMode(GPIO_MODE_INPUT);
  id_bytes[0] = LCD_Read8();
  id_bytes[1] = LCD_Read8();
  id_bytes[2] = LCD_Read8();
  id_bytes[3] = LCD_Read8();
  LCD_SetDataBusMode(GPIO_MODE_OUTPUT_PP);

  LCD_CS_HIGH();
  return (uint16_t)(((uint16_t)id_bytes[2] << 8U) | id_bytes[3]);
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

static void LCD_InitILI9341(void)
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

static void LCD_FillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
  if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
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
  if ((x + width) > LCD_WIDTH)
  {
    width = (int16_t)(LCD_WIDTH - x);
  }
  if ((y + height) > LCD_HEIGHT)
  {
    height = (int16_t)(LCD_HEIGHT - y);
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

static void LCD_FillScreen(uint16_t color)
{
  LCD_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

static bool ButtonPressed(GPIO_TypeDef *port, uint16_t pin)
{
  return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

static void Game_DrawScene(void)
{
  LCD_FillScreen(COLOR_SKY);
  LCD_FillRect(0, GROUND_Y, LCD_WIDTH, LCD_HEIGHT - GROUND_Y, COLOR_GROUND);
  LCD_FillRect(0, GROUND_Y, LCD_WIDTH, 3, COLOR_GROUND_TOP);
}

static void Game_DrawPlayer(int16_t x, int16_t y)
{
  /* Cuerpo */
  LCD_FillRect(x + 2, y + 2, PLAYER_WIDTH - 4, PLAYER_HEIGHT - 4, COLOR_PLAYER);

  /* Cabeza y pies: una silueta sencilla para verificar la orientacion. */
  LCD_FillRect(x + 4, y, PLAYER_WIDTH - 8, 4, COLOR_PLAYER);
  LCD_FillRect(x + 2, y + PLAYER_HEIGHT - 4, 5, 4, COLOR_PLAYER_DARK);
  LCD_FillRect(x + PLAYER_WIDTH - 7, y + PLAYER_HEIGHT - 4, 5, 4, COLOR_PLAYER_DARK);

  /* Ojo hacia la derecha. */
  LCD_FillRect(x + PLAYER_WIDTH - 6, y + 5, 3, 3, COLOR_WHITE);
  LCD_FillRect(x + PLAYER_WIDTH - 4, y + 6, 1, 1, COLOR_BLACK);
}

static void Game_ErasePlayer(int16_t x, int16_t y)
{
  LCD_FillRect(x, y, PLAYER_WIDTH, PLAYER_HEIGHT, COLOR_SKY);
}

static void Game_Init(void)
{
  player.x = (LCD_WIDTH - PLAYER_WIDTH) / 2;
  player.y_fp = (int32_t)(GROUND_Y - PLAYER_HEIGHT) * FP_ONE;
  player.vy_fp = 0L;
  player.on_ground = true;
  jump_was_pressed = false;

  Game_DrawScene();
  Game_DrawPlayer(player.x, (int16_t)(player.y_fp >> FP_SHIFT));
}

static void Game_Update(void)
{
  bool left_pressed = ButtonPressed(BTN_LEFT_GPIO_Port, BTN_LEFT_Pin);
  bool right_pressed = ButtonPressed(BTN_RIGHT_GPIO_Port, BTN_RIGHT_Pin);
  bool jump_pressed = ButtonPressed(BTN_JUMP_GPIO_Port, BTN_JUMP_Pin);

  int16_t old_x = player.x;
  int16_t old_y = (int16_t)(player.y_fp >> FP_SHIFT);

  if (left_pressed && !right_pressed)
  {
    player.x -= PLAYER_SPEED;
  }
  else if (right_pressed && !left_pressed)
  {
    player.x += PLAYER_SPEED;
  }

  if (player.x < 0)
  {
    player.x = 0;
  }
  else if (player.x > (LCD_WIDTH - PLAYER_WIDTH))
  {
    player.x = LCD_WIDTH - PLAYER_WIDTH;
  }

  /* El salto solo inicia con un nuevo toque y estando sobre el suelo. */
  if (jump_pressed && !jump_was_pressed && player.on_ground)
  {
    player.vy_fp = JUMP_SPEED_FP;
    player.on_ground = false;
  }
  jump_was_pressed = jump_pressed;

  if (!player.on_ground)
  {
    player.vy_fp += GRAVITY_FP;
    if (player.vy_fp > MAX_FALL_SPEED_FP)
    {
      player.vy_fp = MAX_FALL_SPEED_FP;
    }

    player.y_fp += player.vy_fp;

    int32_t floor_position = (int32_t)(GROUND_Y - PLAYER_HEIGHT) * FP_ONE;
    if (player.y_fp >= floor_position)
    {
      player.y_fp = floor_position;
      player.vy_fp = 0L;
      player.on_ground = true;
    }
  }

  int16_t new_y = (int16_t)(player.y_fp >> FP_SHIFT);
  if ((player.x != old_x) || (new_y != old_y))
  {
    Game_ErasePlayer(old_x, old_y);
    Game_DrawPlayer(player.x, new_y);
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
  uint16_t lcd_id;
  char uart_message[96];
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  UART_Log("\r\n--- Jump King STM32: inicio ---\r\n");

  /* Mantiene deseleccionados los dispositivos SPI que no utilizamos. */
  HAL_GPIO_WritePin(F_CS_GPIO_Port, F_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  LCD_HardwareReset();
  lcd_id = LCD_ReadID();
  (void)snprintf(uart_message, sizeof(uart_message),
                 "LCD ID leido: 0x%04X\r\n", (unsigned int)lcd_id);
  UART_Log(uart_message);

  if (lcd_id != 0x9341U)
  {
    UART_Log("Aviso: ID distinto de ILI9341; se intentara la inicializacion ILI9341.\r\n");
  }

  LCD_InitILI9341();
  Game_Init();
  UART_Log("Prueba lista: izquierda, derecha y salto.\r\n");

  next_frame = HAL_GetTick() + FRAME_TIME_MS;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    if ((int32_t)(now - next_frame) >= 0)
    {
      Game_Update();
      next_frame += FRAME_TIME_MS;

      /* Evita tratar de recuperar cientos de frames despues de una pausa de debug. */
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
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

  GPIO_InitStruct.Pin = BTN_LEFT_Pin | BTN_RIGHT_Pin | BTN_JUMP_Pin;
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
