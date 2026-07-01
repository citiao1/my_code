/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications - 精简版
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// 引入应用层模块头文件，用于在任务中调用各模块功能
#include "chassic.h"    // 底盘控制模块
#include "control.h"    // 控制模块
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 遥控器数据接收标志位（在main.c的UART IDLE中断中置位）
// 当USART3接收到完整的SBUS数据包后，该标志被置1，通知控制任务可以处理遥控器数据
extern uint8_t Rc_Start;

/* USER CODE END PTD */

/* Private variables ---------------------------------------------------------*/
// FreeRTOS任务句柄：用于管理和控制各个任务
osThreadId defaultTaskHandle;      // 默认任务句柄（当前为空任务）
osThreadId conroltaskHandle;       // 控制任务句柄：处理遥控器命令解析
osThreadId chassistaskHandle;      // 底盘任务句柄：执行底盘控制

/* Private function prototypes -----------------------------------------------*/
void StartDefaultTask(void const * argument);
extern void ControlTask(void const * argument);
extern void ChassisTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
// FreeRTOS静态内存分配支持：为IDLE任务提供静态内存空间
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

/**
 * @brief 获取IDLE任务的静态内存分配
 */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS初始化函数 - 精简版
  * @param  None
  * @retval None
  * 
  * 功能说明：
  * 1. 创建所有应用任务并设置优先级
  * 2. 精简版只创建控制任务和底盘任务
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Create the thread(s) */
  /* 创建所有应用任务 */
  
  /* definition and creation of defaultTask */
  // 默认任务：优先级Normal，堆栈128字（512字节），周期5ms
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of conroltask */
  // 控制任务：优先级Idle（最低），堆栈128字，周期5ms
  // 功能：解析遥控器命令，生成底盘控制命令
  osThreadDef(conroltask, ControlTask, osPriorityIdle, 0, 128);
  conroltaskHandle = osThreadCreate(osThread(conroltask), NULL);

  /* definition and creation of chassistask */
  // 底盘任务：优先级Idle，堆栈128字，周期5ms
  // 功能：执行底盘运动控制（麦轮运动学解算）
  osThreadDef(chassistask, ChassisTask, osPriorityIdle, 0, 128);
  chassistaskHandle = osThreadCreate(osThread(chassistask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */

}

/**
  * @brief  默认任务实现函数
  * @param  argument: 任务参数（未使用）
  * @retval None
  */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  for(;;)
  {
    osDelay(5);    // 延迟5ms
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief  控制任务实现函数 - 精简版
 * @param  argument: 任务参数（未使用）
 * @retval None
 * 
 * 功能说明：
 * 1. 检查遥控器数据是否准备好（Rc_Start标志）
 * 2. 如果数据准备好，调用RoboCmdTask()解析命令
 * 3. RoboCmdTask()会：
 *    - 解析遥控器摇杆数据
 *    - 根据开关设置底盘模式
 *    - 生成底盘控制命令
 * 4. 每5ms执行一次，确保控制命令及时更新
 */
void ControlTask(void const *argument)
{
  while(1)  // 任务必须包含无限循环
  {
    // 检查遥控器数据接收标志
    // Rc_Start在main.c的UART IDLE中断中置位（USART3接收完SBUS数据后）
    if (Rc_Start == 1)
    {  
      // 执行控制任务：解析遥控器命令，生成底盘控制命令
      RoboCmdTask();
    }
    // 延迟5ms，让出CPU给其他任务，控制任务周期为5ms
    vTaskDelay(5);
  }
}

/**
 * @brief  底盘任务实现函数 - 精简版
 * @param  argument: 任务参数（未使用）
 * @retval None
 * 
 * 功能说明：
 * 1. 检查遥控器数据是否准备好（Rc_Start标志）
 * 2. 如果数据准备好，执行底盘控制：
 *    - Chassistask(): 底盘运动控制（麦轮运动学解算）
 *    - 在Chassistask()内部会调用MotorControl()计算PID并发送CAN报文
 * 3. 每5ms执行一次，保证控制频率为200Hz
 */
void ChassisTask(void const *argument)
{
  while(1)  // 任务必须包含无限循环
  {
    // 检查遥控器数据接收标志
    // 只有接收到遥控器数据后，才执行控制任务
    if (Rc_Start == 1)
    {
        // 执行底盘控制：运动学解算，计算四个轮子速度，设置电机参考值
        // 注意：MotorControl()在Chassistask()内部调用，统一处理所有电机
        Chassistask();
    }
    
    // 延迟5ms，控制任务周期为5ms（200Hz控制频率）
    vTaskDelay(5);
  }
}
/* USER CODE END Application */

