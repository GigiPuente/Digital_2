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

typedef enum
{
  Estado_Espera = 0,
  Estado_Cuenta_Regresiva,
  Estado_Carrera,
  Estado_Ganador

} Estado_Juego;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define Cuenta_Max       4
#define Antirrebote      150

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// Estado actual del juego
volatile Estado_Juego estado_juego = Estado_Espera;

// Banderas activadas por las interrupciones
volatile uint8_t flag_inicio = 0;
volatile uint8_t flag_btnJ1 = 0;
volatile uint8_t flag_btnJ2 = 0;

// Variables antirrebote
volatile uint32_t ultimo_tick_inicio = 0;
volatile uint32_t ultimo_tick_J1 = 0;
volatile uint32_t ultimo_tick_J2 = 0;

// Contadores de los jugadores
uint8_t cuentaJ1 = 0;
uint8_t cuentaJ2 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */

void actz_LEDs_J1(uint8_t count);
void actz_LEDs_J2(uint8_t count);

void Display_Apagar(void);
void Display_Numero(uint8_t numero);

void Reset_Juego(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE BEGIN 2 */

  // Iniciar con LEDs y display apagados
  Reset_Juego();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    switch (estado_juego)
    {
      case Estado_Espera:

        // Esperar al boton de inicio
        if (flag_inicio)
        {
          flag_inicio = 0;

          // Reiniciar todo antes de comenzar
          Reset_Juego();

          // Pasar a la cuenta regresiva
          estado_juego = Estado_Cuenta_Regresiva;
        }

        break;


      case Estado_Cuenta_Regresiva:

        // Bloquear cualquier pulsacion previa de los jugadores
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Mostrar 5
        Display_Numero(5);
        HAL_Delay(1000);

        // Ignorar botones durante el conteo
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Mostrar 4
        Display_Numero(4);
        HAL_Delay(1000);

        // Ignorar botones durante el conteo
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Mostrar 3
        Display_Numero(3);
        HAL_Delay(1000);

        // Ignorar botones durante el conteo
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Mostrar 2
        Display_Numero(2);
        HAL_Delay(1000);

        // Ignorar botones durante el conteo
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Mostrar 1
        Display_Numero(1);
        HAL_Delay(1000);

        // Ignorar botones durante el conteo
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Mostrar 0
        Display_Numero(0);
        HAL_Delay(1000);

        // Limpiar cualquier pulsacion realizada durante la cuenta regresiva
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Apagar el display al comenzar la carrera
        Display_Apagar();

        // Habilitar la carrera
        estado_juego = Estado_Carrera;

        break;


      case Estado_Carrera:

        // Revisar boton J1
        if (flag_btnJ1)
        {
          flag_btnJ1 = 0;

          // Incrementar contador J1
          if (cuentaJ1 < Cuenta_Max)
          {
            cuentaJ1++;

            // Actualizar LEDs J1
            actz_LEDs_J1(cuentaJ1);
          }
        }


        // Revisar boton J2
        if (flag_btnJ2)
        {
          flag_btnJ2 = 0;

          // Incrementar contador J2
          if (cuentaJ2 < Cuenta_Max)
          {
            cuentaJ2++;

            // Actualizar LEDs J2
            actz_LEDs_J2(cuentaJ2);
          }
        }


        // Revisar si J1 gano
        if (cuentaJ1 >= Cuenta_Max)
        {
          // Asegurar los cuatro LEDs del ganador encendidos
          actz_LEDs_J1(Cuenta_Max);

          // Apagar todos los LEDs del jugador contrario
          actz_LEDs_J2(0);

          // Mostrar 1 en el display
          Display_Numero(1);

          // Limpiar pulsaciones pendientes
          flag_btnJ1 = 0;
          flag_btnJ2 = 0;

          // Finalizar la carrera
          estado_juego = Estado_Ganador;
        }

        // Revisar si J2 gano
        else if (cuentaJ2 >= Cuenta_Max)
        {
          // Asegurar los cuatro LEDs del ganador encendidos
          actz_LEDs_J2(Cuenta_Max);

          // Apagar todos los LEDs del jugador contrario
          actz_LEDs_J1(0);

          // Mostrar 2 en el display
          Display_Numero(2);

          // Limpiar pulsaciones pendientes
          flag_btnJ1 = 0;
          flag_btnJ2 = 0;

          // Finalizar la carrera
          estado_juego = Estado_Ganador;
        }

        break;


      case Estado_Ganador:

        // Ignorar cualquier pulsacion de los jugadores
        flag_btnJ1 = 0;
        flag_btnJ2 = 0;

        // Permitir iniciar una nueva carrera
        if (flag_inicio)
        {
          flag_inicio = 0;

          // Reiniciar LEDs, contadores y display
          Reset_Juego();

          // Volver a realizar la cuenta regresiva
          estado_juego = Estado_Cuenta_Regresiva;
        }

        break;


      default:

        // Recuperarse de un estado invalido
        Reset_Juego();
        estado_juego = Estado_Espera;

        break;
    }

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
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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

  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, J22_Pin | J21_Pin | F_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, J11_Pin | J12_Pin | J13_Pin | J24_Pin |
                           J23_Pin | D_Pin | E_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, J14_Pin | C_Pin | B_Pin | A_Pin |
                           G_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BtnINICIO_Pin */
  GPIO_InitStruct.Pin = BtnINICIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BtnINICIO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : J22_Pin J21_Pin F_Pin */
  GPIO_InitStruct.Pin = J22_Pin | J21_Pin | F_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : J11_Pin J12_Pin J13_Pin J24_Pin
                           J23_Pin D_Pin E_Pin */
  GPIO_InitStruct.Pin = J11_Pin | J12_Pin | J13_Pin | J24_Pin |
                        J23_Pin | D_Pin | E_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin | USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : J14_Pin C_Pin B_Pin A_Pin
                           G_Pin */
  GPIO_InitStruct.Pin = J14_Pin | C_Pin | B_Pin | A_Pin |
                        G_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BtnJ1_Pin */
  GPIO_InitStruct.Pin = BtnJ1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BtnJ1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BtnJ2_Pin */
  GPIO_InitStruct.Pin = BtnJ2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BtnJ2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// Callback para interrupciones externas
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // Obtener el tiempo actual
  uint32_t now = HAL_GetTick();


  // Interrupcion del boton de inicio
  if (GPIO_Pin == BtnINICIO_Pin)
  {
    // Hacer el antirrebote
    if ((now - ultimo_tick_inicio) > Antirrebote)
    {
      ultimo_tick_inicio = now;
      flag_inicio = 1;
    }
  }


  // Interrupcion del boton J1
  else if (GPIO_Pin == BtnJ1_Pin)
  {
    // Hacer antirrebote
    if ((now - ultimo_tick_J1) > Antirrebote)
    {
      ultimo_tick_J1 = now;

      // Registrar la pulsacion solamente durante la carrera
      if (estado_juego == Estado_Carrera)
      {
        flag_btnJ1 = 1;
      }
    }
  }


  // Interrupcion del boton J2
  else if (GPIO_Pin == BtnJ2_Pin)
  {
    // Hacer antirrebote
    if ((now - ultimo_tick_J2) > Antirrebote)
    {
      ultimo_tick_J2 = now;

      // Registrar la pulsacion solamente durante la carrera
      if (estado_juego == Estado_Carrera)
      {
        flag_btnJ2 = 1;
      }
    }
  }
}


// Apagar todos los segmentos del display
void Display_Apagar(void)
{
  HAL_GPIO_WritePin(
      GPIOB,
      A_Pin | B_Pin | C_Pin | G_Pin,
      GPIO_PIN_RESET
  );

  HAL_GPIO_WritePin(
      GPIOA,
      D_Pin | E_Pin,
      GPIO_PIN_RESET
  );

  HAL_GPIO_WritePin(
      GPIOC,
      F_Pin,
      GPIO_PIN_RESET
  );
}


// Mostrar un numero del 0 al 5 en el display
void Display_Numero(uint8_t numero)
{
  // Apagar el display antes de mostrar el nuevo numero
  Display_Apagar();

  switch (numero)
  {
    case 0:

      // Segmentos A, B, C, D, E y F
      HAL_GPIO_WritePin(
          GPIOB,
          A_Pin | B_Pin | C_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOA,
          D_Pin | E_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOC,
          F_Pin,
          GPIO_PIN_SET
      );

      break;


    case 1:

      // Segmentos B y C
      HAL_GPIO_WritePin(
          GPIOB,
          B_Pin | C_Pin,
          GPIO_PIN_SET
      );

      break;


    case 2:

      // Segmentos A, B, D, E y G
      HAL_GPIO_WritePin(
          GPIOB,
          A_Pin | B_Pin | G_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOA,
          D_Pin | E_Pin,
          GPIO_PIN_SET
      );

      break;


    case 3:

      // Segmentos A, B, C, D y G
      HAL_GPIO_WritePin(
          GPIOB,
          A_Pin | B_Pin | C_Pin | G_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOA,
          D_Pin,
          GPIO_PIN_SET
      );

      break;


    case 4:

      // Segmentos B, C, F y G
      HAL_GPIO_WritePin(
          GPIOB,
          B_Pin | C_Pin | G_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOC,
          F_Pin,
          GPIO_PIN_SET
      );

      break;


    case 5:

      // Segmentos A, C, D, F y G
      HAL_GPIO_WritePin(
          GPIOB,
          A_Pin | C_Pin | G_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOA,
          D_Pin,
          GPIO_PIN_SET
      );

      HAL_GPIO_WritePin(
          GPIOC,
          F_Pin,
          GPIO_PIN_SET
      );

      break;


    default:

      // Apagar si se recibe un numero invalido
      Display_Apagar();

      break;
  }
}


// Actualizar los cuatro LEDs J1
void actz_LEDs_J1(uint8_t count)
{
  // LED 1
  HAL_GPIO_WritePin(
      GPIOA,
      J11_Pin,
      (count >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );

  // LED 2
  HAL_GPIO_WritePin(
      GPIOA,
      J12_Pin,
      (count >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );

  // LED 3
  HAL_GPIO_WritePin(
      GPIOA,
      J13_Pin,
      (count >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );

  // LED 4
  HAL_GPIO_WritePin(
      GPIOB,
      J14_Pin,
      (count >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );
}


// Actualizar los cuatro LEDs J2
void actz_LEDs_J2(uint8_t count)
{
  // LED 1
  HAL_GPIO_WritePin(
      GPIOC,
      J21_Pin,
      (count >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );

  // LED 2
  HAL_GPIO_WritePin(
      GPIOC,
      J22_Pin,
      (count >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );

  // LED 3
  HAL_GPIO_WritePin(
      GPIOA,
      J23_Pin,
      (count >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );

  // LED 4
  HAL_GPIO_WritePin(
      GPIOA,
      J24_Pin,
      (count >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET
  );
}


// Reiniciar el juego
void Reset_Juego(void)
{
  // Reiniciar los contadores
  cuentaJ1 = 0;
  cuentaJ2 = 0;

  // Limpiar los flags de los jugadores
  flag_btnJ1 = 0;
  flag_btnJ2 = 0;

  // Apagar todos los LEDs
  actz_LEDs_J1(0);
  actz_LEDs_J2(0);

  // Apagar el display
  Display_Apagar();
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
