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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#include "stm32f4xx_nucleo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BtnINICIO_Pin GPIO_PIN_13
#define BtnINICIO_GPIO_Port GPIOC
#define BtnINICIO_EXTI_IRQn EXTI15_10_IRQn
#define J22_Pin GPIO_PIN_0
#define J22_GPIO_Port GPIOC
#define J21_Pin GPIO_PIN_1
#define J21_GPIO_Port GPIOC
#define J11_Pin GPIO_PIN_0
#define J11_GPIO_Port GPIOA
#define J12_Pin GPIO_PIN_1
#define J12_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define J13_Pin GPIO_PIN_4
#define J13_GPIO_Port GPIOA
#define J24_Pin GPIO_PIN_6
#define J24_GPIO_Port GPIOA
#define J23_Pin GPIO_PIN_7
#define J23_GPIO_Port GPIOA
#define J14_Pin GPIO_PIN_0
#define J14_GPIO_Port GPIOB
#define C_Pin GPIO_PIN_10
#define C_GPIO_Port GPIOB
#define F_Pin GPIO_PIN_7
#define F_GPIO_Port GPIOC
#define D_Pin GPIO_PIN_8
#define D_GPIO_Port GPIOA
#define E_Pin GPIO_PIN_9
#define E_GPIO_Port GPIOA
#define BtnJ1_Pin GPIO_PIN_10
#define BtnJ1_GPIO_Port GPIOA
#define BtnJ1_EXTI_IRQn EXTI15_10_IRQn
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define BtnJ2_Pin GPIO_PIN_3
#define BtnJ2_GPIO_Port GPIOB
#define BtnJ2_EXTI_IRQn EXTI3_IRQn
#define B_Pin GPIO_PIN_4
#define B_GPIO_Port GPIOB
#define A_Pin GPIO_PIN_5
#define A_GPIO_Port GPIOB
#define G_Pin GPIO_PIN_6
#define G_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
