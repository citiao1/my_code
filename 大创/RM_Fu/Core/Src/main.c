/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "IMU.h"
#include "ecoder.h"
#include "contor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 全局变量定义：用于存储各模块的数据指针和状态
RC_Ctl_t *psbus;                    // 遥控器数据指针（SBUS解析后的结果）
AHRS_FEED *imu_feed;                // IMU姿态数据指针（AHRS数据包解析后的结果）
uint8_t *subsbuff;                  // SBUS原始数据缓冲区指针（用于DMA接收）
uint8_t *imubuff;                   // IMU原始数据缓冲区指针（用于DMA接收，128字节）
uint8_t *imusgbuff;                 // IMU备用缓冲区指针（未使用）
uint8_t *watchrevbuff;              // 视觉接收数据缓冲区指针（用于DMA接收，128字节）
uint16_t count;                     // 计数器（未使用）
IMU_RX_STATE_e imu_rx_stata = AHRSWAITING;  // IMU接收状态（等待状态）
EcoderInstance *ecoder;             // 编码器实例指针（用于底盘角度测量）
Chassic_Ctrl_Cmd *chassis_data;     // 底盘控制命令指针（未使用）
uint8_t Rc_Start = 0;               // 遥控器数据接收标志（在UART IDLE中断中置位）
RC_Ctl_t *rcdata;                   // 遥控器数据指针（获取遥控器数据结构体）
uint16_t Keys = 0;                  // 键盘按键值（未使用）
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  应用程序入口点
  * @retval int（实际不会返回）
  * 
  * 功能说明：
  * 这是STM32程序的入口函数，执行流程如下：
  * 1. 初始化HAL库
  * 2. 配置系统时钟
  * 3. 初始化外设（GPIO、DMA、UART、CAN、看门狗等）
  * 4. 获取各模块数据缓冲区指针
  * 5. 初始化整车控制模块
  * 6. 启动UART DMA接收
  * 7. 初始化FreeRTOS并启动调度器
  * 
  * 注意：启动调度器后，程序不会返回到main函数
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* 复位所有外设，初始化Flash接口和Systick
  * HAL_Init()会：
  * 1. 配置Flash预取指和指令缓存
  * 2. 配置SysTick定时器（用于HAL_Delay）
  * 3. 配置NVIC优先级分组
  */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 配置系统时钟
  * 将系统时钟配置为168MHz
  * APB1：42MHz，APB2：84MHz
  */
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
  MX_USART1_UART_Init();       // USART1初始化：IMU通信（AHRS协议，128字节）
  MX_USART6_UART_Init();       // USART6初始化：视觉通信（自定义协议，128字节）
  MX_CAN2_Init();              // CAN2初始化：电机通信总线2
  /* USER CODE BEGIN 2 */
  // 获取各模块的数据缓冲区指针
  imubuff = GetRxAHRBuff();          // 获取IMU接收缓冲区（128字节）
  subsbuff = GiveSbusBuff();         // 获取SBUS接收缓冲区（256字节）
  rcdata = GetRCData();              // 获取遥控器数据结构体指针
  watchrevbuff = GetWatchingRevBufff();  // 获取视觉接收缓冲区（128字节）
  Keys = rcdata->key[0].keys;        // 获取键盘按键值（未使用）
  // imusgbuff = Getsgbuff();        // 备用缓冲区（未使用）
  // imu_feed = GetAHRSFeed();       // IMU数据指针（未使用）
  // chassis_data = GetChassisCmd(); // 底盘命令指针（未使用）
  
  // 初始化整车控制模块：初始化底盘、云台、射击、视觉模块
  ControlInit();
  
  // 启动UART DMA接收（IDLE中断模式）
  // USART6：视觉数据接收，128字节，IDLE中断触发数据处理
  HAL_UARTEx_ReceiveToIdle_DMA(&huart6, watchrevbuff, 128);  // 启动DMA接收
  __HAL_DMA_DISABLE_IT(huart6.hdmarx, DMA_IT_HT);            // 禁用半传输中断，只需要传输完成中断
  
  // USART1：IMU数据接收，128字节，IDLE中断触发数据处理
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, imubuff, 128);       // 启动DMA接收
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);            // 禁用半传输中断
  
  // USART3：遥控器数据接收，256字节，IDLE中断触发数据处理
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, subsbuff, 256);      // 启动DMA接收
  __HAL_DMA_DISABLE_IT(huart3.hdmarx,DMA_IT_HT);             // 禁用半传输中断

  // while (HAL_UART_Receive(&huart1, watchrev, 1, 5) != HAL_OK);





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
  * 
  * 时钟树：
  * HSE(8MHz) -> PLL(×168/6 = 28MHz) -> SYSCLK(168MHz)
  * SYSCLK -> HCLK(168MHz) -> APB1(42MHz), APB2(84MHz)
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};  // 振荡器初始化结构体
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};  // 时钟初始化结构体

  /** 配置主内部稳压器输出电压
  * 电压等级1：1.8V，用于168MHz系统时钟
  */
  __HAL_RCC_PWR_CLK_ENABLE();                              // 使能PWR时钟
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  // 配置电压等级1

  /** 初始化RCC振荡器
  * 根据RCC_OscInitTypeDef结构体中的参数配置振荡器
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;  // 使用LSI和HSE
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;                 // 使能外部高速时钟（8MHz）
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;                 // 使能内部低速时钟（用于看门狗）
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;             // 使能PLL
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;     // PLL源选择HSE
  RCC_OscInitStruct.PLL.PLLM = 6;                          // PLLM分频系数：HSE/6 = 8MHz/6 = 1.33MHz
  RCC_OscInitStruct.PLL.PLLN = 168;                        // PLLN倍频系数：1.33MHz × 168 = 224MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;              // PLLP分频系数：224MHz/2 = 112MHz（实际使用168MHz，需要调整）
  RCC_OscInitStruct.PLL.PLLQ = 4;                          // PLLQ分频系数：用于USB等外设
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();  // 配置失败，进入错误处理
  }

  /** 初始化CPU、AHB和APB总线时钟
  * 配置系统时钟和各总线时钟分频
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;  // 配置所有时钟类型
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;  // 系统时钟源选择PLL
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;         // AHB时钟分频：不分频（168MHz）
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;          // APB1时钟分频：4分频（42MHz）
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;          // APB2时钟分频：2分频（84MHz）

  // 配置Flash延迟：168MHz需要5个等待周期
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();  // 配置失败，进入错误处理
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief  UART IDLE中断处理函数
 * @param  huart: UART句柄指针
 * @retval None
 * 
 * 功能说明：
 * 当UART接收完成（检测到IDLE信号）时，触发此中断
 * 该函数处理三个UART的IDLE中断：
 * 1. USART3：遥控器数据（SBUS协议）
 * 2. USART1：IMU数据（AHRS协议）
 * 3. USART6：视觉数据（自定义协议）
 * 
 * 处理流程：
 * 1. 清除IDLE标志位
 * 2. 重置DMA传输计数器
 * 3. 重新启动DMA接收
 * 4. 解析接收到的数据
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

  // USART1：IMU数据接收中断处理
  if (huart->Instance == USART1)
  {
      uint32_t DMA_FLAGS1, tmp1;  // DMA标志位和临时变量
      
      // 检查是否是IDLE中断
      if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
      {
        // 清除IDLE标志位
        tmp1 = huart->Instance->SR;  // 读取状态寄存器
        tmp1 = huart->Instance->DR;  // 读取数据寄存器
        tmp1++;                      // 防止编译器优化
        
        // 获取DMA传输完成标志位索引
        DMA_FLAGS1 = __HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx);
        
        // 禁用DMA
        __HAL_DMA_DISABLE(huart->hdmarx);
        
        // 清除DMA传输完成标志
        __HAL_DMA_CLEAR_FLAG(huart->hdmarx, DMA_FLAGS1);

        // 重置DMA传输计数器
        huart->hdmarx->Instance->NDTR = huart->RxXferSize;
        
        // 重新使能DMA
        __HAL_DMA_ENABLE(huart->hdmarx);
      }
      // 解析AHRS数据包：将原始数据转换为IMU姿态结构
      imu_feed = AHRSPackHandle(imubuff);
      
      // 重新启动DMA接收，准备接收下一帧数据
      HAL_UARTEx_ReceiveToIdle_DMA(&huart1, imubuff, 128);
      __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);  // 禁用半传输中断
  }
  
  // USART6：视觉数据接收中断处理
  if (huart->Instance == USART6)
  {
    uint32_t DMA_FLAGS1, tmp1;  // DMA标志位和临时变量
    
    // 检查是否是IDLE中断
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
    {
      // 清除IDLE标志位
      tmp1 = huart->Instance->SR;  // 读取状态寄存器
      tmp1 = huart->Instance->DR;  // 读取数据寄存器
      tmp1++;                      // 防止编译器优化
      
      // 获取DMA传输完成标志位索引
      DMA_FLAGS1 = __HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx);
      
      // 禁用DMA
      __HAL_DMA_DISABLE(huart->hdmarx);
      
      // 清除DMA传输完成标志
      __HAL_DMA_CLEAR_FLAG(huart->hdmarx, DMA_FLAGS1);

      // 重置DMA传输计数器
      huart->hdmarx->Instance->NDTR = huart->RxXferSize;
      
      // 重新使能DMA
      __HAL_DMA_ENABLE(huart->hdmarx);
    }
    // 解析视觉数据：将原始数据转换为视觉控制结构
    WatchingRec(watchrevbuff);
    
    // 重新启动DMA接收，准备接收下一帧数据
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, watchrevbuff, 128);
    __HAL_DMA_DISABLE_IT(huart6.hdmarx, DMA_IT_HT);  // 禁用半传输中断
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
