
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_WIDTH   320U
#define LCD_HEIGHT  240U
#define LCD_MADCTL  0x28U  //Horizontal. Usar 0xE8 para girar 180 grados.
#define LDR_LIGHT_ON   3000U
#define LDR_LIGHT_OFF  2600U
#define I2C_A_ADDRESS 0x31U
#define NEOPIXEL_COUNT 4U
#define NEOPIXEL_RESET_SLOTS 240U
#define NEOPIXEL_ZERO 34U
#define NEOPIXEL_ONE 68U
#define NEOPIXEL_FRAME_SIZE (NEOPIXEL_COUNT * 24U + NEOPIXEL_RESET_SLOTS)

#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8U) << 8) | \
                                   (((g) & 0xFCU) << 3) | \
                                   (((b) & 0xF8U) >> 3)))
#define C_ROAD   RGB565(38U, 50U, 56U)
#define C_BAY    RGB565(214U, 217U, 218U)
#define C_LINE   RGB565(239U, 196U, 56U)
#define C_TEXT   RGB565(30U, 41U, 48U)
#define C_WHITE  RGB565(255U, 255U, 255U)
#define C_FREE   RGB565(22U, 101U, 52U)
#define C_CAR    RGB565(223U, 70U, 70U)
#define C_ROOF   RGB565(241U, 123U, 114U)
#define C_WHEEL  RGB565(23U, 33U, 38U)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch4_up;

/* USER CODE BEGIN PV */
//P1 a P4 usan LDR; P5 a P8 reciben su estado del ESP32.
static volatile uint8_t parkingOccupied[8] = {0U};
//DMA escribe lecturas de 12 bits en este arreglo de medias palabras.
static volatile uint16_t ldrRaw[4] = {0U};
//Cada valor indica la duracion en alto de un bit; al final van 300 us en bajo.
static uint16_t neopixelFrame[NEOPIXEL_FRAME_SIZE] = {0U};
static volatile uint8_t neopixelFinished = 0U;
//El ESP32 lee tres bytes y escribe cuatro bytes con los ocho espacios.
static uint8_t i2cLocal[3] = {0x5AU, 0U, 0x5AU};
static uint8_t i2cGlobalRx[4] = {0U};
static volatile uint32_t i2cGlobalFrame = 0U;
static uint8_t i2cReady = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//El primer LED corresponde a P1, el cuarto a P4; el orden de color es GRB.
static void NeoPixel_Show(void)
{
  uint16_t index = 0U;

  for (uint8_t slot = 0U; slot < NEOPIXEL_COUNT; ++slot)
  {
    uint8_t green = (parkingOccupied[slot] == 0U) ? 32U : 0U;
    uint8_t red = (parkingOccupied[slot] == 0U) ? 0U : 32U;
    uint32_t grb = ((uint32_t)green << 16) | ((uint32_t)red << 8);

    for (uint8_t bit = 0U; bit < 24U; ++bit)
    {
      neopixelFrame[index++] = ((grb & (1UL << (23U - bit))) != 0U)
                              ? NEOPIXEL_ONE : NEOPIXEL_ZERO;
    }
  }

  while (index < NEOPIXEL_FRAME_SIZE) { neopixelFrame[index++] = 0U; }

  neopixelFinished = 0U;
  if (HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_4,
                            (uint32_t *)neopixelFrame,
                            (uint16_t)NEOPIXEL_FRAME_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  uint32_t start = HAL_GetTick();
  while (neopixelFinished == 0U)
  {
    if ((HAL_GetTick() - start) > 10U) { Error_Handler(); }
  }

  if (HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) { neopixelFinished = 1U; }
}

//TFT ILI9341 de 8 bits: D0=PA9, D1=PC7, D2=PA10, D3=PB3.
//D4=PB5, D5=PB4, D6=PB10, D7=PA8.
//RD=PA0, WR=PA1, RS=PA4, CS=PB0, RST=PC1.
//RD permanece en alto; solo escribimos en la pantalla.
static void LCD_Write8(uint8_t value)
{
  HAL_GPIO_WritePin(GPIOA, LCD_D0_Pin, (value & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, LCD_D1_Pin, (value & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LCD_D2_Pin, (value & 0x04U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LCD_D3_Pin, (value & 0x08U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LCD_D4_Pin, (value & 0x10U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LCD_D5_Pin, (value & 0x20U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LCD_D6_Pin, (value & 0x40U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LCD_D7_Pin, (value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);

  __NOP(); __NOP(); __NOP(); __NOP();
  HAL_GPIO_WritePin(GPIOA, LCD_WR_Pin, GPIO_PIN_RESET);
  __NOP(); __NOP(); __NOP(); __NOP();
  HAL_GPIO_WritePin(GPIOA, LCD_WR_Pin, GPIO_PIN_SET);
  __NOP(); __NOP(); __NOP(); __NOP();
}

//Deja CS activo y RS en modo datos despues del comando.
static void LCD_BeginCommand(uint8_t command)
{
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LCD_RS_Pin, GPIO_PIN_RESET);
  LCD_Write8(command);
  HAL_GPIO_WritePin(GPIOA, LCD_RS_Pin, GPIO_PIN_SET);
}

static void LCD_EndWrite(void)
{
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}

static void LCD_WriteRegister(uint8_t command, const uint8_t *data, uint8_t count)
{
  LCD_BeginCommand(command);
  for (uint8_t i = 0U; i < count; ++i)
  {
    LCD_Write8(data[i]);
  }
  LCD_EndWrite();
}

static void LCD_Init(void)
{
  //Formato: comando, cantidad de parametros y parametros.
  static const uint8_t init[] = {
    0xEF, 3, 0x03, 0x80, 0x02,
    0xCF, 3, 0x00, 0xC1, 0x30,
    0xED, 4, 0x64, 0x03, 0x12, 0x81,
    0xE8, 3, 0x85, 0x00, 0x78,
    0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
    0xF7, 1, 0x20,
    0xEA, 2, 0x00, 0x00,
    0xC0, 1, 0x23,
    0xC1, 1, 0x10,
    0xC5, 2, 0x3E, 0x28,
    0xC7, 1, 0x86,
    0x36, 1, LCD_MADCTL,
    0x3A, 1, 0x55,
    0xB1, 2, 0x00, 0x18,
    0xB6, 3, 0x08, 0x82, 0x27,
    0xF2, 1, 0x00,
    0x26, 1, 0x01,
    0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
              0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
  };

  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin | LCD_WR_Pin | LCD_RS_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(5U);
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20U);
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(150U);

  LCD_WriteRegister(0x01U, 0, 0U); //Reinicio por software.
  HAL_Delay(150U);

  uint32_t i = 0U;
  while (i < sizeof(init))
  {
    uint8_t command = init[i++];
    uint8_t count = init[i++];
    LCD_WriteRegister(command, &init[i], count);
    i += count;
  }

  LCD_WriteRegister(0x11U, 0, 0U); //Salir de reposo.
  HAL_Delay(150U);
  LCD_WriteRegister(0x29U, 0, 0U); //Encender la pantalla.
  HAL_Delay(20U);
}

//Dibuja un rectangulo recortado a los limites de la pantalla.
static void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         uint16_t color)
{
  if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT) || (w == 0U) || (h == 0U))
  {
    return;
  }
  if (w > LCD_WIDTH - x)  { w = LCD_WIDTH - x; }
  if (h > LCD_HEIGHT - y) { h = LCD_HEIGHT - y; }

  uint16_t xEnd = x + w - 1U;
  uint16_t yEnd = y + h - 1U;
  uint8_t coordinates[4];

  coordinates[0] = (uint8_t)(x >> 8);
  coordinates[1] = (uint8_t)x;
  coordinates[2] = (uint8_t)(xEnd >> 8);
  coordinates[3] = (uint8_t)xEnd;
  LCD_WriteRegister(0x2AU, coordinates, 4U);

  coordinates[0] = (uint8_t)(y >> 8);
  coordinates[1] = (uint8_t)y;
  coordinates[2] = (uint8_t)(yEnd >> 8);
  coordinates[3] = (uint8_t)yEnd;
  LCD_WriteRegister(0x2BU, coordinates, 4U);

  LCD_BeginCommand(0x2CU);
  uint32_t pixels = (uint32_t)w * h;
  while (pixels-- != 0U)
  {
    LCD_Write8((uint8_t)(color >> 8));
    LCD_Write8((uint8_t)color);
  }
  LCD_EndWrite();
}

//Fuente 5x7 con los caracteres usados por la interfaz.
static void LCD_DrawText(uint16_t x, uint16_t y, const char *text,
                         uint16_t color, uint8_t scale)
{
  static const char characters[] = "0123456789PBLIRES:/ ";
  static const uint8_t glyphs[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, //0
    {0x00,0x42,0x7F,0x40,0x00}, //1
    {0x42,0x61,0x51,0x49,0x46}, //2
    {0x21,0x41,0x45,0x4B,0x31}, //3
    {0x18,0x14,0x12,0x7F,0x10}, //4
    {0x27,0x45,0x45,0x45,0x39}, //5
    {0x3C,0x4A,0x49,0x49,0x30}, //6
    {0x01,0x71,0x09,0x05,0x03}, //7
    {0x36,0x49,0x49,0x49,0x36}, //8
    {0x06,0x49,0x49,0x29,0x1E}, //9
    {0x7F,0x09,0x09,0x09,0x06}, //P
    {0x7F,0x49,0x49,0x49,0x36}, //B
    {0x7F,0x40,0x40,0x40,0x40}, //L
    {0x00,0x41,0x7F,0x41,0x00}, //I
    {0x7F,0x09,0x19,0x29,0x46}, //R
    {0x7F,0x49,0x49,0x49,0x41}, //E
    {0x46,0x49,0x49,0x49,0x31}, //S
    {0x00,0x36,0x36,0x00,0x00}, //:
    {0x20,0x10,0x08,0x04,0x02}, //Barra
    {0x00,0x00,0x00,0x00,0x00}  //Espacio
  };

  while (*text != '\0')
  {
    uint8_t index = 0U;
    while ((characters[index] != '\0') && (characters[index] != *text))
    {
      ++index;
    }

    if (characters[index] != '\0')
    {
      for (uint8_t column = 0U; column < 5U; ++column)
      {
        for (uint8_t row = 0U; row < 7U; ++row)
        {
          if ((glyphs[index][column] & (1U << row)) != 0U)
          {
            LCD_FillRect(x + column * scale, y + row * scale,
                         scale, scale, color);
          }
        }
      }
    }

    x += 6U * scale;
    ++text;
  }
}

//Carro visto desde arriba, hecho con rectangulos.
static void Parking_DrawCar(uint16_t x, uint16_t y, uint8_t facesDown)
{
  LCD_FillRect(x - 3U, y + 8U, 4U, 11U, C_WHEEL);
  LCD_FillRect(x + 27U, y + 8U, 4U, 11U, C_WHEEL);
  LCD_FillRect(x - 3U, y + 36U, 4U, 11U, C_WHEEL);
  LCD_FillRect(x + 27U, y + 36U, 4U, 11U, C_WHEEL);
  LCD_FillRect(x, y, 28U, 56U, C_CAR);
  LCD_FillRect(x + 4U, y + (facesDown ? 6U : 40U), 20U, 7U, C_ROAD);
  LCD_FillRect(x + 4U, y + 19U, 20U, 17U, C_ROOF);
  LCD_FillRect(x + 3U, y + (facesDown ? 40U : 6U), 22U, 9U, C_ROAD);
}

static void Parking_DrawSpace(uint8_t slot)
{
  char label[] = "P1";
  uint8_t top = (slot < 4U);
  uint16_t left = 8U + (slot % 4U) * 76U;
  uint16_t centerX = left + 38U;

  //Limpiar solo el interior de este espacio para conservar sus lineas.
  LCD_FillRect(left + 2U, top ? 10U : 146U, 72U, 84U, C_BAY);
  label[1] = (char)('1' + slot);
  LCD_DrawText(centerX - 12U, top ? 15U : 213U, label, C_TEXT, 2U);

  if (parkingOccupied[slot] != 0U)
  {
    Parking_DrawCar(centerX - 14U, top ? 32U : 152U, top);
  }
  else
  {
    LCD_DrawText(centerX - 30U, top ? 54U : 173U, "LIBRE", C_FREE, 2U);
  }
}

static void Parking_DrawCounter(void)
{
  uint8_t freeSpaces = 0U;
  char counter[] = "LIBRES: 0/8";

  for (uint8_t slot = 0U; slot < 8U; ++slot)
  {
    if (parkingOccupied[slot] == 0U) { ++freeSpaces; }
  }

  counter[8] = (char)('0' + freeSpaces);
  LCD_FillRect(94U, 113U, 132U, 14U, C_ROAD);
  LCD_DrawText(94U, 113U, counter, C_WHITE, 2U);
}

//Lee las cuatro LDR: al tapar un sensor, su lectura baja y el espacio se ocupa.
static void Parking_UpdateSensors(void)
{
  uint8_t changed = 0U;

  for (uint8_t slot = 0U; slot < 4U; ++slot)
  {
    uint16_t reading = ldrRaw[slot];
    uint8_t occupied = parkingOccupied[slot];

    //Dos umbrales evitan que el dibujo oscile con pequenas variaciones.
    if (reading <= LDR_LIGHT_OFF) { occupied = 1U; }
    else if (reading >= LDR_LIGHT_ON) { occupied = 0U; }

    if (occupied != parkingOccupied[slot])
    {
      parkingOccupied[slot] = occupied;
      Parking_DrawSpace(slot);
      changed = 1U;
    }
  }

  if (changed != 0U)
  {
    Parking_DrawCounter();
    NeoPixel_Show();
  }
}

//Actualiza P5 a P8 solamente cuando el ESP32 confirma que conoce su estado.
static void Parking_UpdateGlobal(void)
{
  uint32_t frame = i2cGlobalFrame;
  uint8_t changed = 0U;

  if ((frame & 0x10000UL) == 0U) { return; }

  for (uint8_t slot = 4U; slot < 8U; ++slot)
  {
    if ((frame & (1UL << (slot + 8U))) == 0U) { continue; }
    uint8_t occupied = ((frame & (1UL << slot)) != 0U) ? 1U : 0U;
    if (parkingOccupied[slot] != occupied)
    {
      parkingOccupied[slot] = occupied;
      Parking_DrawSpace(slot);
      changed = 1U;
    }
  }

  if (changed != 0U) { Parking_DrawCounter(); }
}

static void Parking_Draw(void)
{
  LCD_FillRect(0U, 0U, LCD_WIDTH, LCD_HEIGHT, C_ROAD);

  //Cuatro espacios arriba y otros cuatro abajo del pasillo.
  LCD_FillRect(8U, 8U, 304U, 88U, C_BAY);
  LCD_FillRect(8U, 144U, 304U, 88U, C_BAY);
  LCD_FillRect(8U, 8U, 304U, 2U, C_LINE);
  LCD_FillRect(8U, 230U, 304U, 2U, C_LINE);

  for (uint8_t column = 0U; column <= 4U; ++column)
  {
    uint16_t x = 8U + column * 76U;
    if (column == 4U) { x -= 2U; }
    LCD_FillRect(x, 8U, 2U, 88U, C_LINE);
    LCD_FillRect(x, 144U, 2U, 88U, C_LINE);
  }

  for (uint8_t slot = 0U; slot < 8U; ++slot) { Parking_DrawSpace(slot); }
  Parking_DrawCounter();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  //Dibujar todos los espacios vacios al iniciar.
  LCD_Init();
  Parking_Draw();
  //La pantalla arranca antes de intentar habilitar I2C.
  MX_I2C1_Init();
  //Cada conversion ADC corresponde a PC2, PC3, PC4 y PC5, en ese orden.
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ldrRaw, 4U) != HAL_OK)
  {
    Error_Handler();
  }
  //DMA sigue en modo circular; la CPU revisa los valores cada 100 ms.
  __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT | DMA_IT_TC);
  //Al inicio los cuatro NeoPixel muestran los espacios libres en verde.
  NeoPixel_Show();
  //La direccion de esta STM32 en el bus I2C es 0x31.
  if (i2cReady != 0U) { (void)HAL_I2C_EnableListen_IT(&hi2c1); }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Parking_UpdateSensors();
    Parking_UpdateGlobal();
    if ((i2cReady != 0U) &&
        (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY))
    {
      (void)HAL_I2C_EnableListen_IT(&hi2c1);
    }
    HAL_Delay(100U);
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
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

//Inicializacion de I2C1 como esclavo de 7 bits en la direccion 0x31.
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000U;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = (I2C_A_ADDRESS << 1);
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0x62U;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  i2cReady = (HAL_I2C_Init(&hi2c1) == HAL_OK) ? 1U : 0U;
}

//Inicializacion de TIM3 para el envio de datos a los NeoPixel.
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 112;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN TIM3_Init 2 */
  //El DMA cambia el pulso despues de cada bit; el registro se aplica al siguiente.
  TIM3->CCMR2 |= TIM_CCMR2_OC4PE;
  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  //Interrupcion del DMA de TIM3.
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  HAL_GPIO_WritePin(GPIOA, LCD_RS_Pin|SPI1_SCK_Pin|SPI1_MISO_Pin|SPI1_MOSI_Pin
                          |LCD_D7_Pin|LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

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

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin SPI1_SCK_Pin
                           SPI1_MISO_Pin SPI1_MOSI_Pin LCD_D7_Pin LCD_D0_Pin
                           LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|SPI1_SCK_Pin
                          |SPI1_MISO_Pin|SPI1_MOSI_Pin|LCD_D7_Pin|LCD_D0_Pin
                          |LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
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
//Cuando el ESP32 lee, entregamos la ocupacion local de P1 a P4.
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c,
                          uint8_t TransferDirection,
                          uint16_t AddrMatchCode)
{
  (void)AddrMatchCode;
  if (hi2c->Instance != I2C1) { return; }

  if (TransferDirection == I2C_DIRECTION_TRANSMIT)
  {
    (void)HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2cGlobalRx,
                                        4U, I2C_FIRST_AND_LAST_FRAME);
  }
  else
  {
    uint8_t mask = 0U;
    for (uint8_t slot = 0U; slot < 4U; ++slot)
    {
      if (parkingOccupied[slot] != 0U) { mask |= (uint8_t)(1U << slot); }
    }
    i2cLocal[0] = 0x5AU;
    i2cLocal[1] = mask;
    i2cLocal[2] = (uint8_t)(0x5AU ^ mask);
    (void)HAL_I2C_Slave_Seq_Transmit_IT(hi2c, i2cLocal,
                                         3U, I2C_FIRST_AND_LAST_FRAME);
  }
}

//Publica la trama global verificada para actualizar la pantalla en el bucle.
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != I2C1) { return; }
  if (i2cGlobalRx[0] != 0xA5U) { return; }
  if (i2cGlobalRx[3] !=
      (uint8_t)(0xA5U ^ i2cGlobalRx[1] ^ i2cGlobalRx[2])) { return; }

  i2cGlobalFrame = 0x10000UL |
      ((uint32_t)i2cGlobalRx[2] << 8) | i2cGlobalRx[1];
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1) { (void)HAL_I2C_EnableListen_IT(hi2c); }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  if ((hi2c->Instance == I2C1) &&
      (HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_READY))
  {
    (void)HAL_I2C_EnableListen_IT(hi2c);
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

