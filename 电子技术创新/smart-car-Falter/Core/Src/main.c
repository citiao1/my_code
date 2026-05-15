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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "app.h"
#include "motor.h"
#include "servo.h"
#include "ultrasonic.h"
#include "oled.h"
#include "font.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE BEGIN PV */
// 超声波输入捕获变量：记录三个通道(CH1-左, CH2-中, CH3-右)的上升沿和下降沿时刻
uint16_t upEdge1 = 0, upEdge2 = 0, upEdge3 = 0;
uint16_t downEdge1 = 0, downEdge2 = 0, downEdge3 = 0;
/* USER CODE END PV */
// 标记当前通道是否处于等待上升沿状态 (1: Rising, 0: Falling)
uint8_t is_rising1 = 1;
uint8_t is_rising2 = 1;
uint8_t is_rising3 = 1;

// 计算后的原始距离值 (cm)
volatile float distance1 = 0;
volatile float distance2 = 0;
volatile float distance3 = 0;

// 经过应用层处理或滤波后用于控制的距离值 (默认400表示无障碍)
volatile float dist1 = 400;
volatile float dist2 = 400;
volatile float dist3 = 400;

char buf[20];
char str[30];
int upEdge = 0;
int downEdge = 0;
volatile float distance = 0;
// 记录各通道最后一次有效测距的时间戳，用于超时判断
volatile uint32_t g_distance_time_1,g_distance_time_2,g_distance_time_3;
volatile float g_distance = 0;

uint32_t out_time=0;
uint8_t out_flg=0;
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	// 电机 PWM
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

	// 舵机 PWM
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

	// 超声波 输入捕获
	HAL_TIM_Base_Start(&htim3);
	HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
	HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);
	
	//App初始化
	App_Init();
	//oled初始化
	HAL_Delay(100);              // 等待 OLED 上电稳定
	OLED_Init();                 // 初始化 OLED 显示屏
	OLED_DisPlay_On(); 
	         // 打开显示（可选，默认 OLED_Init 已打开）

	//Motor_SetSpeed(100,100);
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		Ultrasonic_Trig();	// 触发超声波测距
		OLED_NewFrame();
		
		// 更新OLED显示内容：左、右、中距离及电机速度、当前状态
		sprintf(str, "%.3f left   ",dist1);//左
    OLED_PrintString(0, 0, str, &font16x16, OLED_COLOR_NORMAL);
		sprintf(str, "%.3f right   ", dist3);//右边
    OLED_PrintString(0, 25, str, &font16x16, OLED_COLOR_NORMAL);
		sprintf(str, "%.3f middle    ", dist2);//中间
		OLED_PrintString(0, 50, str, &font16x16, OLED_COLOR_NORMAL);
		sprintf(str, "%d %d", left_speed,right_speed);//电机速度
		OLED_PrintString(0, 12, str, &font16x16, OLED_COLOR_NORMAL);
		OLED_PrintString(0, 37, state, &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
		


		App_Update(dist1,dist2,dist3);    // 核心控制循环：处理传感器数据并更新电机状态    

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief 定时器输入捕获回调函数，用于计算超声波高电平脉宽并转换为距离
  * @param htim 定时器句柄
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	uint16_t pulse_width = 0;
	// 通道1 (左侧超声波)
	if(htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
	{
		if(is_rising1){
			upEdge1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
			is_rising1 = 0;
		}else{
			downEdge1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
			is_rising1 = 1;
			
			// 计算脉宽并转换为距离 (cm): 脉宽(us) * 0.017
			uint16_t pulse_width = (uint16_t)(downEdge1 - upEdge1);
      distance1 = pulse_width * 0.017f; 
      g_distance_time_1 = HAL_GetTick();
		}
	// 通道2 (中间超声波)
	}else if(htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
	{
		if(is_rising2){
			upEdge2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
			is_rising2 = 0;
		}else{
			downEdge2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
			is_rising2 = 1;
			uint16_t pulse_width = (uint16_t)(downEdge2 - upEdge2);
      distance2 = pulse_width * 0.017f;
      g_distance_time_2 = HAL_GetTick();
		}
	// 通道3 (右侧超声波)
	}else if(htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
	{
		if(is_rising3){
			upEdge3 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_3, TIM_INPUTCHANNELPOLARITY_FALLING);
			is_rising3 = 0;
		}else{
			downEdge3 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_3, TIM_INPUTCHANNELPOLARITY_RISING);
			is_rising3 = 1;
			
			uint16_t pulse_width = (uint16_t)(downEdge3 - upEdge3);
      distance3 = pulse_width * 0.017f;
      g_distance_time_3 = HAL_GetTick();
			
		}
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
