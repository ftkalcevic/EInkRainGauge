/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f1xx_hal.h"

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
extern ADC_HandleTypeDef hadc1;

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GPIO_EINK_Reset_Pin GPIO_PIN_1
#define GPIO_EINK_Reset_GPIO_Port GPIOA
#define GPIO_EINK_DC_Pin GPIO_PIN_2
#define GPIO_EINK_DC_GPIO_Port GPIOA
#define GPIO_EINK_Busy_Pin GPIO_PIN_3
#define GPIO_EINK_Busy_GPIO_Port GPIOA
#define SPI_EINK_CS_Pin GPIO_PIN_4
#define SPI_EINK_CS_GPIO_Port GPIOA
#define SPI_EINK_CO_Pin GPIO_PIN_5
#define SPI_EINK_CO_GPIO_Port GPIOA
#define SPI_EINK_DO_Pin GPIO_PIN_7
#define SPI_EINK_DO_GPIO_Port GPIOA
#define ADC1_VSOLAR_Pin GPIO_PIN_0
#define ADC1_VSOLAR_GPIO_Port GPIOB
#define ADC1_VBAT_Pin GPIO_PIN_1
#define ADC1_VBAT_GPIO_Port GPIOB
#define GPIO_BUTTON1_Pin GPIO_PIN_12
#define GPIO_BUTTON1_GPIO_Port GPIOB
#define GPIO_BUTTON1_EXTI_IRQn EXTI15_10_IRQn
#define GPIO_BUTTON2_Pin GPIO_PIN_13
#define GPIO_BUTTON2_GPIO_Port GPIOB
#define GPIO_BUTTON2_EXTI_IRQn EXTI15_10_IRQn
#define GPIO_BUCKET_Pin GPIO_PIN_14
#define GPIO_BUCKET_GPIO_Port GPIOB
#define GPIO_BUCKET_EXTI_IRQn EXTI15_10_IRQn
/* USER CODE BEGIN Private defines */
#define RST_GPIO_Port	GPIO_EINK_Reset_GPIO_Port
#define RST_Pin			GPIO_EINK_Reset_Pin
#define DC_GPIO_Port	GPIO_EINK_DC_GPIO_Port
#define DC_Pin			GPIO_EINK_DC_Pin
#define PWR_GPIO_Port	
#define PWR_Pin			
#define SPI_CS_GPIO_Port	SPI_EINK_CS_GPIO_Port
#define SPI_CS_Pin		SPI_EINK_CS_Pin
#define BUSY_GPIO_Port	GPIO_EINK_Busy_GPIO_Port
#define BUSY_Pin		GPIO_EINK_Busy_Pin
#define DIN_GPIO_Port	SPI_EINK_DO_GPIO_Port
#define DIN_Pin			SPI_EINK_DO_Pin
#define SCK_GPIO_Port	SPI_EINK_CO_GPIO_Port
#define SCK_Pin			SPI_EINK_CO_Pin
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
