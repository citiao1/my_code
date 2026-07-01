/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pid.h"
#include "mycan.h"
#include "dr16.h"
#include <string.h>
#include "imu.h"
#include "chassic.h"
#include "conter.h"
#include "gimbal.h"
#include "shoot.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t *sbus_buffer; 
RC_Ctl_t *pbus;
RC_Ctl_t *rcdata;
MotorData *motor;
uint8_t *imubuff;
AHRS_FEED *imu_feed;
//建立中断函数参数指针，需要赋予地址避免成为野指针
//射击三个电机（1个2006--0x205，两个3508--0x206、0x207）使用0x1ff标识符，二维云台使用0x2ff（两个6020）,底盘使用0x200（四个3508,0x201--0x204）
//共九个电机

///
PidConfig PidData[6]=
	{
	{.kp=1.1,.ki= 0.03,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=2.0,.ki= 0.01,.kd=0,.out_max=25000,.i_max=100,.dead_zone=10,.improve = PID_Integral_limit, //| PID_ChangingIntegrationRate,
	.Output_LPF_RC = 0.5,},
	{.kp=180,.ki= 0.01,.kd=0,.out_max=5000,.i_max=100,.dead_zone=10,.improve = PID_Integral_limit, //| PID_ChangingIntegrationRate,
	.Output_LPF_RC = 0.5,},
	};
	//测试使用的垃圾不用管
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_DMA_Init();
  MX_CAN1_Init();
  MX_USART3_UART_Init();
  MX_TIM2_Init();
  MX_IWDG_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  
   ////
	sbus_buffer = GiveBuffer();
	rcdata = GetRCData();
	motor = GetMotorData();
	imubuff = GiveAHRSBuffer();
	imu_feed = GetAHSRFeed();
	////用于中断指针参数赋予地址
	MycanInit();
	///
	GlobalInit();
	///
	PidAllInit(6,PidData);	
	
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, imubuff, 128);
	__HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);	
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, sbus_buffer, 256);
	__HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT); 
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_IDLE_IRQHandler(UART_HandleTypeDef *huart)
{
    /* 1. 确认中断源为USART3 */
  if (huart->Instance == USART3)
  {	
	  HAL_IWDG_Refresh(&hiwdg);
	uint32_t DMA_FLAGS3,tmp3;
        /* 2. 获取DMA接收句柄 */
       if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
		{
		tmp3=huart->Instance->SR;
		tmp3=huart->Instance->DR;
		tmp3++;
        DMA_FLAGS3 = __HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx);
        __HAL_DMA_DISABLE(huart->hdmarx);
		__HAL_DMA_CLEAR_FLAG(huart->hdmarx, DMA_FLAGS3);
		huart->hdmarx->Instance->NDTR = huart->RxXferSize;
		__HAL_DMA_ENABLE(huart->hdmarx);
        }
	RemoteDataProcess(sbus_buffer); 
    }
  if (huart->Instance == USART1)
	{
		uint32_t DMA_FLAGS1, tmp1;
		if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
		{
		tmp1 = huart->Instance->SR;
		tmp1 = huart->Instance->DR;
		tmp1++;
		DMA_FLAGS1 = __HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx);
		__HAL_DMA_DISABLE(huart->hdmarx);
		__HAL_DMA_CLEAR_FLAG(huart->hdmarx, DMA_FLAGS1);
	
		huart->hdmarx->Instance->NDTR = huart->RxXferSize;
		__HAL_DMA_ENABLE(huart->hdmarx);
		}
		AHRSPackHandle(imubuff);
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, imubuff, 128);
		__HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
	}
} 
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
	if(htim->Instance==TIM2)
	{
//	float angle = (rcdata->rc.ch1 - 1024.0f) * 180.0f/660.0f;
//	PidReturn(&PidData[0].pid,-5000,motor[4].speed);
//	PidReturn(&PidData[5].pid,-90.0f,motor[8].total_angle);
//	PidReturn(&PidData[4].pid,PidData[5].pid.out,motor[8].speed);
//	CanMotorTransmit(0x2ff,0,PidData[4].pid.out,0,0);
//	CanMotorTransmit(0x1ff,0,PidData[0].pid.out,0,0);
//		CanMotorTransmit(0x1ff,0,0,PidData[0].pid.out,0);
//		CanMotorTransmit(0x1ff,PidData[0].pid.out,0,0,0);
		GlobalModeSelect();
//		ChassicControl();
//		GimbalControl();
		ShootControl();
	}
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
