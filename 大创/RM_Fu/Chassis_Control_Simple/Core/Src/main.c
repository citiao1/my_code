/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - 精简版（仅遥控器控制底盘）
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "cmsis_os.h"
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DR16.h"
#include "control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 全局变量定义：用于存储各模块的数据指针和状态
RC_Ctl_t *psbus;                    // 遥控器数据指针（SBUS解析后的结果）
uint8_t *subsbuff;                  // SBUS原始数据缓冲区指针（用于DMA接收）
uint8_t Rc_Start = 0;               // 遥控器数据接收标志（在UART IDLE中断中置位）
RC_Ctl_t *rcdata;                   // 遥控器数据指针（获取遥控器数据结构体）
/* USER CODE END PTD */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  应用程序入口点 - 精简版
  * @retval int（实际不会返回）
  * 
  * 功能说明：
  * 这是STM32程序的入口函数，执行流程如下：
  * 1. 初始化HAL库
  * 2. 配置系统时钟
  * 3. 初始化外设（GPIO、DMA、UART3、CAN1、看门狗）
  * 4. 获取遥控器数据缓冲区指针
  * 5. 初始化控制模块（底盘初始化）
  * 6. 启动UART3 DMA接收（遥控器）
  * 7. 初始化FreeRTOS并启动调度器
  * 
  * 注意：启动调度器后，程序不会返回到main函数
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* 复位所有外设，初始化Flash接口和Systick */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 配置系统时钟：168MHz */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  // 初始化所有配置的外设
  MX_GPIO_Init();              // GPIO初始化：配置LED、按键等GPIO引脚
  MX_DMA_Init();               // DMA初始化：配置DMA控制器，用于UART和CAN的数据传输
  MX_USART3_UART_Init();       // USART3初始化：遥控器通信（SBUS协议，256字节）
  MX_IWDG_Init();              // 看门狗初始化：防止程序死锁，需要定期喂狗
  MX_CAN1_Init();              // CAN1初始化：电机通信总线1
  /* USER CODE BEGIN 2 */
  // 获取遥控器数据缓冲区指针
  subsbuff = GiveSbusBuff();         // 获取SBUS接收缓冲区（256字节）
  rcdata = GetRCData();              // 获取遥控器数据结构体指针
  
  // 初始化控制模块：初始化底盘模块（注册电机、配置PID等）
  ControlInit();
  
  // 启动UART DMA接收（IDLE中断模式）
  // USART3：遥控器数据接收，256字节，IDLE中断触发数据处理
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, subsbuff, 256);      // 启动DMA接收
  __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);             // 禁用半传输中断，只需要传输完成中断

  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
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
  * @brief  系统时钟配置函数
  * @retval None
  * 
  * 功能说明：
  * 配置STM32F407的系统时钟，包括：
  * 1. 外部高速时钟（HSE）：8MHz
  * 2. 内部低速时钟（LSI）：用于看门狗
  * 3. PLL倍频：将HSE倍频到168MHz
  * 4. 系统时钟：168MHz
  * 5. APB1时钟：42MHz（HCLK/4）
  * 6. APB2时钟：84MHz（HCLK/2）
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** 配置主内部稳压器输出电压 */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** 初始化RCC振荡器 */
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

  /** 初始化CPU、AHB和APB总线时钟 */
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
/**
 * @brief  UART IDLE中断处理函数 - 精简版
 * @param  huart: UART句柄指针
 * @retval None
 * 
 * 功能说明：
 * 当UART接收完成（检测到IDLE信号）时，触发此中断
 * 精简版只处理USART3的IDLE中断（遥控器数据）
 * 
 * 处理流程：
 * 1. 清除IDLE标志位
 * 2. 重置DMA传输计数器
 * 3. 重新启动DMA接收
 * 4. 解析接收到的SBUS数据
 */
void HAL_UART_IDLE_IRQHandler(UART_HandleTypeDef *huart)
{
  // USART3：遥控器数据接收中断处理
  if (huart->Instance == USART3)
  {
    Rc_Start = 1;  // 置位遥控器数据接收标志，通知控制任务处理数据
    uint32_t DMA_FLAGS3, tmp3;  // DMA标志位和临时变量
    HAL_IWDG_Refresh(&hiwdg);   // 喂狗：刷新看门狗，防止系统复位
    
    // 检查是否是IDLE中断（数据接收完成）
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
    {
      // 清除IDLE标志位：读取SR和DR寄存器以清除标志
      tmp3 = huart->Instance->SR;  // 读取状态寄存器
      tmp3 = huart->Instance->DR;  // 读取数据寄存器
      tmp3++;                      // 防止编译器优化
      
      // 获取DMA传输完成标志位索引
      DMA_FLAGS3 = __HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx);
      
      // 禁用DMA，准备重置
      __HAL_DMA_DISABLE(huart->hdmarx);
      
      // 清除DMA传输完成标志
      __HAL_DMA_CLEAR_FLAG(huart->hdmarx, DMA_FLAGS3);

      // 重置DMA传输计数器为原始值
      huart->hdmarx->Instance->NDTR = huart->RxXferSize;
      
      // 重新使能DMA，开始下一次接收
      __HAL_DMA_ENABLE(huart->hdmarx);
    }
    // 解析SBUS数据：将原始数据转换为遥控器控制结构
    psbus = Dbus_to_rc(subsbuff);
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

