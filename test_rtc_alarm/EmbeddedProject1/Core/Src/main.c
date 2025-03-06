/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void SuspendFastSemihostingPolling();
void ResumeFastSemihostingPolling();

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint32_t RTC_ReadTimeCounter(RTC_HandleTypeDef *hrtc)
{
	uint16_t high1 = 0U, high2 = 0U, low = 0U;
	uint32_t timecounter = 0U;

	high1 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);
	low   = READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT);
	high2 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);

	if (high1 != high2)
	{
		/* In this case the counter roll over during reading of CNTL and CNTH registers,
		   read again CNTL register then return the counter value */
		timecounter = (((uint32_t) high2 << 16U) | READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT));
	}
	else
	{
		/* No counter roll over during reading of CNTL and CNTH registers, counter
		   value is equal to first value of CNTL and CNTH */
		timecounter = (((uint32_t) high1 << 16U) | low);
	}

	return timecounter;
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
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	RTC_AlarmTypeDef alarm = { 0 };
	alarm.Alarm = RTC_ALARM_A;
	alarm.AlarmTime.Hours = 0;
	alarm.AlarmTime.Minutes = 0;
	alarm.AlarmTime.Seconds = 10;
	//HAL_RTC_SetAlarm(&hrtc, &alarm, RTC_FORMAT_BIN); 
//	for (;;)
//	{
//		printf("tick\n");
//		HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, GPIO_PIN_SET);
//		HAL_Delay(500);
//		HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, GPIO_PIN_RESET);
//		HAL_Delay(500);
//	}
	//HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, GPIO_PIN_SET);
	int n = 1;
  while (1)
  {
		if (__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_SEC))
		{
			__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_SEC);
			n++;
			printf("%d:%d\n", n, RTC_ReadTimeCounter(&hrtc));
			//HAL_GPIO_TogglePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin);
		  }
//		if (__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_ALRAF))
//		{
//			__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
//			printf("Alarm!!\n");
		  
	  if (n % 10 == 0)
	  {
		  n++;
		  RTC_AlarmTypeDef alarm;
		  HAL_RTC_GetTime(&hrtc, &(alarm.AlarmTime), RTC_FORMAT_BIN);
		  alarm.AlarmTime.Seconds += 10;
		  if (alarm.AlarmTime.Seconds >= 60)
		  {
			  alarm.AlarmTime.Seconds -= 60;
			  alarm.AlarmTime.Minutes++;
			  if (alarm.AlarmTime.Minutes >= 60)
			  {
				  alarm.AlarmTime.Minutes -= 60;
				  alarm.AlarmTime.Hours++;
			  }
		  }
		  
		  printf("Setting alarm %02d:%02d\n", alarm.AlarmTime.Minutes, alarm.AlarmTime.Seconds);
		  printf("Low power stop\n"); // fflush(stdout); HAL_Delay(250);
		  //HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, GPIO_PIN_SET);
		  SuspendFastSemihostingPolling();

		  
		  
		  //Then just proceed to set up the new alarm and go to sleep
		  HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);
		  HAL_PWR_EnableSEVOnPend();
		  HAL_SuspendTick();
		  //HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFE);
		  //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
		  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
		  //SystemClock_Config();
		  HAL_ResumeTick();	
		//SystemClock_Config();
		  //__HAL_RCC_BKP_CLK_ENABLE();
		  
		  //MODIFY_REG(PWR->CR, PWR_CR_LPDS, PWR_MAINREGULATOR_ON); 
		  
		  //HAL_RCCEx_PeriphCLKConfig(RCC_PERIPHCLK_RTC for
		  //        PeriphClockSelection and select RTCClockSelection(LSE, LSI or HSE)
		  //    (+) Enable the BKP clock in using __HAL_RCC_BKP_CLK_ENABLE()
			  
		  //MX_RTC_Init(0);
		  //MX_GPIO_Init();
		  
		  //HAL_PWR_EnableBkUpAccess();
		  //__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
		  //HAL_PWR_DisableBkUpAccess();		  
		  //__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
		  //__HAL_RTC_ALARM_DISABLE_IT(&hrtc, RTC_IT_ALRA);
		  
		  ResumeFastSemihostingPolling();
		  printf("Wake from Low power stop\n");
		  
		  //HAL_RTC_DeInit(&hrtc);
		  //HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, GPIO_PIN_SET);
		  //HAL_RTC_Init(&hrtc);
		  
		  
#if 0		  
		  HAL_RTC_SetAlarm(&hrtc, &alarm, RTC_FORMAT_BIN);
		  HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);
			
		  // Sleep
		  printf("Low power stop\n"); // fflush(stdout); HAL_Delay(250);
		  SuspendFastSemihostingPolling();
		  HAL_SuspendTick();
		  //__HAL_RCC_PWR_CLK_ENABLE();
		  HAL_PWR_EnableSEVOnPend();
		  //HAL_PWR_EnableSleepOnExit();
		  //__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
		
		  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
			
		  SystemClock_Config();
		  HAL_ResumeTick();
		  __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
		  //__HAL_RCC_PWR_CLK_ENABLE();
		  //HAL_PWR_EnableBkUpAccess();
		  //__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
		  //HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
	  
//			EXTI_HandleTypeDef exti17;
//			exti17.Line = EXTI_LINE_17;
//			exti17.PendingCallback = NULL;
//			HAL_EXTI_ClearConfigLine(&exti17);

			ResumeFastSemihostingPolling();
			printf("Wake from Low power stop\n");
			
//			//MX_RTC_Init();
//			HAL_RTC_GetAlarm(&hrtc, &alarm, RTC_ALARM_A, RTC_FORMAT_BIN);
//			alarm.AlarmTime.Seconds += 10;
//			if (alarm.AlarmTime.Seconds >= 60)
//			{
//				alarm.AlarmTime.Seconds -= 60;
//				alarm.AlarmTime.Minutes++;
//				if (alarm.AlarmTime.Minutes >= 60)
//				{
//					alarm.AlarmTime.Minutes -= 60;
//					alarm.AlarmTime.Hours++;
//				}
//			}
//			HAL_RTC_SetAlarm(&hrtc, &alarm, RTC_FORMAT_BIN);
#endif
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 1;
  DateToUpdate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
