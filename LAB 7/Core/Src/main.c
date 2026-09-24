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
#include <string.h>
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  uint16_t frecuencia;
  uint8_t divisor;
  uint8_t puntillo;
} Nota;
/* USER CODE END PTD */
/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MELODIA_DAC_BPM 140U
#define MELODIA_PWM_BPM 192U
#define RELOJ_PWM_HZ 1000000U
/* USER CODE END PD */
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
uint8_t comandoRx;
uint8_t comandoPendiente = 0;
uint32_t ciclosPorUs;

static const Nota melodiaHarryPotter[] =
{
  {988, 8, 0},
  {1319, 8, 1},
  {1568, 16, 0},
  {1480, 8, 0},
  {1319, 4, 0},
  {1976, 8, 0},
  {1760, 4, 1},
  {1480, 4, 1},
  {1319, 8, 0},
  {1568, 16, 0},
  {1480, 8, 0},
  {1175, 4, 0},
  {1397, 8, 0},
  {988, 2, 0},
  {0, 8, 0},
  {988, 8, 0},
  {1319, 8, 1},
  {1568, 16, 0},
  {1480, 8, 0},
  {1319, 4, 0},
  {1976, 8, 0},
  {2349, 4, 0},
  {2217, 8, 0},
  {2093, 4, 0},
  {1661, 8, 0},
  {2093, 8, 1},
  {1976, 16, 0},
  {1865, 8, 0},
  {932, 4, 0},
  {1568, 8, 0},
  {1319, 2, 0}
};

static const Nota melodiaOdaAlegria[] =
{
  {659, 4, 0},
  {659, 4, 0},
  {698, 4, 0},
  {784, 4, 0},
  {784, 4, 0},
  {698, 4, 0},
  {659, 4, 0},
  {587, 4, 0},
  {523, 4, 0},
  {523, 4, 0},
  {587, 4, 0},
  {659, 4, 0},
  {659, 4, 1},
  {587, 8, 0},
  {587, 2, 0},
  {659, 4, 0},
  {659, 4, 0},
  {698, 4, 0},
  {784, 4, 0},
  {784, 4, 0},
  {698, 4, 0},
  {659, 4, 0},
  {587, 4, 0},
  {523, 4, 0},
  {523, 4, 0},
  {587, 4, 0},
  {659, 4, 0},
  {587, 4, 1},
  {523, 8, 0},
  {523, 2, 0}
};
/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DAC_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
static void UART_Enviar(const char *texto);
static void MostrarMenu(void);
static void DWT_Init(void);
static void EsperarUs(uint32_t tiempoUs);
static void ReproducirTonoDAC(uint16_t frecuencia, uint32_t duracionMs);
static void ReproducirMelodiaDAC(void);
static void ReproducirTonoPWM(uint16_t frecuencia, uint32_t duracionMs);
static void ReproducirMelodiaPWM(void);
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void UART_Enviar(const char *texto)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)texto,
                    (uint16_t)strlen(texto), HAL_MAX_DELAY);
}

static void MostrarMenu(void)
{
  UART_Enviar(
      "\r\n"
      "============================\r\n"
      "     REPRODUCTOR DE AUDIO\r\n"
      "============================\r\n"
      "1. Harry Potter - DAC\r\n"
      "2. Oda a la Alegria - PWM\r\n"
      "============================\r\n"
      "Ingrese 1 o 2 y presione Enter:\r\n> ");
}

static void DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  ciclosPorUs = HAL_RCC_GetHCLKFreq() / 1000000U;
}

static void EsperarUs(uint32_t tiempoUs)
{
  uint32_t inicio = DWT->CYCCNT;
  uint32_t ciclos = tiempoUs * ciclosPorUs;

  while ((DWT->CYCCNT - inicio) < ciclos)
  {
  }
}

static void ReproducirTonoDAC(uint16_t frecuencia, uint32_t duracionMs)
{
  uint32_t medioPeriodoUs;
  uint32_t cambios;
  uint32_t i;

  if (frecuencia == 0)
  {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
    HAL_Delay(duracionMs);
    return;
  }

  medioPeriodoUs = 500000U / frecuencia;
  cambios = (duracionMs * 1000U) / medioPeriodoUs;

  for (i = 0; i < cambios; i++)
  {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R,
                     (i & 1U) ? 4095U : 0U);
    EsperarUs(medioPeriodoUs);
  }

  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
}

static void ReproducirMelodiaDAC(void)
{
  uint32_t i;
  uint32_t duracionMs;
  uint32_t sonidoMs;

  if (HAL_DAC_Start(&hdac, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  for (i = 0; i < (sizeof(melodiaHarryPotter) / sizeof(melodiaHarryPotter[0])); i++)
  {
    duracionMs = (240000U / MELODIA_DAC_BPM) / melodiaHarryPotter[i].divisor;

    if (melodiaHarryPotter[i].puntillo != 0)
    {
      duracionMs += duracionMs / 2U;
    }

    if (melodiaHarryPotter[i].frecuencia == 0)
    {
      ReproducirTonoDAC(0, duracionMs);
    }
    else
    {
      sonidoMs = (duracionMs * 9U) / 10U;
      ReproducirTonoDAC(melodiaHarryPotter[i].frecuencia, sonidoMs);
      ReproducirTonoDAC(0, duracionMs - sonidoMs);
    }
  }

  HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
}

static void ReproducirTonoPWM(uint16_t frecuencia, uint32_t duracionMs)
{
  uint32_t periodo;

  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);

  if (frecuencia == 0)
  {
    HAL_Delay(duracionMs);
    return;
  }

  periodo = RELOJ_PWM_HZ / frecuencia;

  __HAL_TIM_SET_AUTORELOAD(&htim3, periodo - 1U);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, periodo / 2U);
  __HAL_TIM_SET_COUNTER(&htim3, 0);

  if (HAL_TIM_GenerateEvent(&htim3, TIM_EVENTSOURCE_UPDATE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_Delay(duracionMs);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

static void ReproducirMelodiaPWM(void)
{
  uint32_t i;
  uint32_t duracionMs;
  uint32_t sonidoMs;

  for (i = 0; i < (sizeof(melodiaOdaAlegria) / sizeof(melodiaOdaAlegria[0])); i++)
  {
    duracionMs = (240000U / MELODIA_PWM_BPM) / melodiaOdaAlegria[i].divisor;

    if (melodiaOdaAlegria[i].puntillo != 0)
    {
      duracionMs += duracionMs / 2U;
    }

    if (melodiaOdaAlegria[i].frecuencia == 0)
    {
      ReproducirTonoPWM(0, duracionMs);
    }
    else
    {
      sonidoMs = (duracionMs * 9U) / 10U;
      ReproducirTonoPWM(melodiaOdaAlegria[i].frecuencia, sonidoMs);
      ReproducirTonoPWM(0, duracionMs - sonidoMs);
    }
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
  MX_DAC_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  //Esperar terminal
  HAL_Delay(1000);

  //Iniciar contador de ciclos
  DWT_Init();

  //Mostrar menu inicial
  MostrarMenu();
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    if (HAL_UART_Receive(&huart2, &comandoRx, 1, HAL_MAX_DELAY) == HAL_OK)
    {
      switch (comandoRx)
      {
        case '1':
        case '2':
          //Guardar comando
          comandoPendiente = comandoRx;
          break;

        case '\r':
        case '\n':
          if (comandoPendiente == 0)
          {
            break;
          }

          //Procesar comando
          if (comandoPendiente == '1')
          {
            UART_Enviar("\r\nReproduciendo Harry Potter\r\n");

            //Reproducir melodia
            ReproducirMelodiaDAC();

            UART_Enviar("\r\nReproduccion terminada\r\n");
          }
          else
          {
            UART_Enviar("\r\nReproduciendo Oda a la Alegria\r\n");

            //Reproducir melodia
            ReproducirMelodiaPWM();

            UART_Enviar("\r\nReproduccion terminada\r\n");
          }

          comandoPendiente = 0;
          MostrarMenu();
          break;

        default:
          comandoPendiente = 0;
          UART_Enviar("\r\nCaracter invalido\r\n> ");
          break;
      }
    }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}
/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{
  /* USER CODE BEGIN DAC_Init 0 */
  /* USER CODE END DAC_Init 0 */
  DAC_ChannelConfTypeDef sConfig = {0};
  /* USER CODE BEGIN DAC_Init 1 */
  /* USER CODE END DAC_Init 1 */
  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */
  /* USER CODE END DAC_Init 2 */
}
/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  /* USER CODE BEGIN TIM3_Init 0 */
  /* USER CODE END TIM3_Init 0 */
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  /* USER CODE BEGIN TIM3_Init 1 */
  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
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
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */
  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);
}
/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
  /* USER CODE BEGIN TIM6_Init 0 */
  /* USER CODE END TIM6_Init 0 */
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  /* USER CODE BEGIN TIM6_Init 1 */
  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 11249;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */
  /* USER CODE END TIM6_Init 2 */
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
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
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);
  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}
/* USER CODE BEGIN 4 */
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
