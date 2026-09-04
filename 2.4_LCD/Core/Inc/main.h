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
#include "stm32g0xx_hal.h"

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
#define CAN_TX_LED_Pin GPIO_PIN_13
#define CAN_TX_LED_GPIO_Port GPIOC
#define LCD_BL_Pin GPIO_PIN_1
#define LCD_BL_GPIO_Port GPIOA
#define DISP_RESET_Pin GPIO_PIN_2
#define DISP_RESET_GPIO_Port GPIOA
#define DISP_DCX_Pin GPIO_PIN_3
#define DISP_DCX_GPIO_Port GPIOA
#define SPI1_NCS_Pin GPIO_PIN_4
#define SPI1_NCS_GPIO_Port GPIOA
#define cKEY0_Pin GPIO_PIN_0
#define cKEY0_GPIO_Port GPIOB
#define cKEY1_Pin GPIO_PIN_1
#define cKEY1_GPIO_Port GPIOB
#define cKEY2_Pin GPIO_PIN_2
#define cKEY2_GPIO_Port GPIOB
#define cKEY10_Pin GPIO_PIN_10
#define cKEY10_GPIO_Port GPIOB
#define cKEY11_Pin GPIO_PIN_11
#define cKEY11_GPIO_Port GPIOB
#define SPI2_NCS_Pin GPIO_PIN_12
#define SPI2_NCS_GPIO_Port GPIOB
#define DISP_TE_Pin GPIO_PIN_8
#define DISP_TE_GPIO_Port GPIOA
#define DISP_TE_EXTI_IRQn EXTI4_15_IRQn
#define CAN_RX_LED_Pin GPIO_PIN_6
#define CAN_RX_LED_GPIO_Port GPIOC
#define CAN_ERR_LED_Pin GPIO_PIN_7
#define CAN_ERR_LED_GPIO_Port GPIOC
#define KEY_BL_Pin GPIO_PIN_15
#define KEY_BL_GPIO_Port GPIOA
#define RUN_LED_Pin GPIO_PIN_2
#define RUN_LED_GPIO_Port GPIOD
#define cKEY3_Pin GPIO_PIN_3
#define cKEY3_GPIO_Port GPIOB
#define cKEY4_Pin GPIO_PIN_4
#define cKEY4_GPIO_Port GPIOB
#define cKEY5_Pin GPIO_PIN_5
#define cKEY5_GPIO_Port GPIOB
#define cKEY6_Pin GPIO_PIN_6
#define cKEY6_GPIO_Port GPIOB
#define cKEY7_Pin GPIO_PIN_7
#define cKEY7_GPIO_Port GPIOB
#define cKEY8_Pin GPIO_PIN_8
#define cKEY8_GPIO_Port GPIOB
#define cKEY9_Pin GPIO_PIN_9
#define cKEY9_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
