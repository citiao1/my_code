/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "contor.h"     // 整车控制模块
#include "gimbal.h"     // 云台控制模块
#include "watching.h"   // 视觉通信模块
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 遥控器数据接收标志位（在main.c的UART IDLE中断中置位）
// 当USART3接收到完整的SBUS数据包后，该标志被置1，通知控制任务可以处理遥控器数据
extern uint8_t Rc_Start;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
// FreeRTOS任务句柄：用于管理和控制各个任务
osThreadId defaultTaskHandle;      // 默认任务句柄（当前为空任务）
osThreadId conroltaskHandle;       // 控制任务句柄：处理遥控器命令解析和模式切换
osThreadId chassistaskHandle;      // 底盘任务句柄：执行云台、底盘、射击控制
osThreadId watchingtaskHandle;     // 视觉任务句柄：处理与视觉模块的通信

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
extern void ControlTask(void const * argument);
extern void ChassisTask(void const * argument);
extern void WatchingTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
// FreeRTOS静态内存分配支持：
// 为IDLE任务（空闲任务）提供静态内存空间，避免使用动态内存分配
// IDLE任务是FreeRTOS的系统任务，当没有其他任务运行时执行
static StaticTask_t xIdleTaskTCBBuffer;                                    // IDLE任务的TCB（任务控制块）缓冲区
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];                   // IDLE任务的堆栈空间

/**
 * @brief 获取IDLE任务的静态内存分配
 * @param ppxIdleTaskTCBBuffer 指向TCB缓冲区指针的指针（输出参数）
 * @param ppxIdleTaskStackBuffer 指向堆栈缓冲区指针的指针（输出参数）
 * @param pulIdleTaskStackSize 指向堆栈大小的指针（输出参数）
 * @retval None
 * 
 * 说明：FreeRTOS在创建IDLE任务前会调用此函数获取静态内存
 *       这样可以避免使用动态内存分配，提高系统可靠性
 */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;                              // 返回TCB缓冲区地址
  *ppxIdleTaskStackBuffer = &xIdleStack[0];                                 // 返回堆栈起始地址
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;                         // 返回堆栈大小（单位：字）
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS初始化函数
  * @param  None
  * @retval None
  * 
  * 功能说明：
  * 1. 初始化FreeRTOS的同步原语（互斥量、信号量、定时器、队列等）
  * 2. 创建所有应用任务并设置优先级
  * 3. 该函数在main()中调用，在启动调度器之前执行
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* 互斥量（Mutex）初始化区域
   * 互斥量用于保护共享资源，防止多任务同时访问
   * 例如：可以用于保护CAN发送函数，避免多个任务同时发送导致冲突
   */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* 信号量（Semaphore）初始化区域
   * 信号量用于任务间的同步和通信
   * 例如：可以用于通知任务数据已准备好
   */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* 定时器初始化区域
   * FreeRTOS软件定时器，可以周期性执行回调函数
   * 例如：可以用于周期性发送状态信息
   */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* 消息队列初始化区域
   * 队列用于任务间传递数据
   * 例如：可以用于传递传感器数据或控制命令
   */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* 创建所有应用任务 */
  
  /* definition and creation of defaultTask */
  // 默认任务：优先级Normal，堆栈128字（512字节），周期5ms
  // 当前为空任务，可以作为预留任务或系统监控任务使用
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);    // 定义任务属性
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);          // 创建任务并获取句柄

  /* definition and creation of conroltask */
  // 控制任务：优先级Idle（最低），堆栈128字，周期5ms
  // 功能：解析遥控器/键鼠命令，进行模式切换，生成控制命令
  // 优先级设为Idle是因为控制任务不需要实时性，只在遥控器数据准备好时执行
  osThreadDef(conroltask, ControlTask, osPriorityIdle, 0, 128);            // 定义任务属性
  conroltaskHandle = osThreadCreate(osThread(conroltask), NULL);            // 创建任务并获取句柄

  /* definition and creation of chassistask */
  // 底盘任务：优先级Idle，堆栈128字，周期5ms
  // 功能：执行云台控制、底盘运动控制、射击控制
  // 该任务整合了三个模块的控制，在一个周期内顺序执行，保证控制同步性
  osThreadDef(chassistask, ChassisTask, osPriorityIdle, 0, 128);           // 定义任务属性
  chassistaskHandle = osThreadCreate(osThread(chassistask), NULL);          // 创建任务并获取句柄

  /* definition and creation of watchingtask */
  // 视觉任务：优先级High（较高），堆栈128字，周期100ms
  // 功能：与视觉识别模块通信，发送云台姿态数据，接收目标角度信息
  // 优先级设为High是因为视觉通信对实时性要求较高，但周期较长（100ms）
  osThreadDef(watchingtask, WatchingTask, osPriorityHigh, 0, 128);         // 定义任务属性
  watchingtaskHandle = osThreadCreate(osThread(watchingtask), NULL);        // 创建任务并获取句柄

  /* USER CODE BEGIN RTOS_THREADS */
  /* 可以在此添加更多任务 */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  默认任务实现函数
  * @param  argument: 任务参数（未使用）
  * @retval None
  * 
  * 说明：当前为空任务，每5ms执行一次
  *       可以用于系统监控、调试信息输出等功能
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* 无限循环：任务必须包含无限循环，否则任务会退出 */
  for(;;)
  {
    osDelay(5);    // 延迟5ms，让出CPU给其他任务，控制任务周期为5ms
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief  控制任务实现函数
 * @param  argument: 任务参数（未使用）
 * @retval None
 * 
 * 功能说明：
 * 1. 检查遥控器数据是否准备好（Rc_Start标志）
 * 2. 如果数据准备好，调用RoboCmdTask()解析命令
 * 3. RoboCmdTask()会：
 *    - 处理模式切换（R键：遥控器/键鼠切换）
 *    - 根据模式调用RcControlSet()或MouseControlSet()
 *    - 生成底盘、云台、射击的控制命令
 * 4. 每5ms执行一次，确保控制命令及时更新
 * 
 * 注意：该任务优先级为Idle，只有在没有高优先级任务时才运行
 *       这保证了控制任务不会影响实时性要求更高的任务
 */
void ControlTask(void const *argument)
{
  while(1)  // 任务必须包含无限循环
  {
    // 检查遥控器数据接收标志
    // Rc_Start在main.c的UART IDLE中断中置位（USART3接收完SBUS数据后）
    if (Rc_Start == 1)
    {  
      // 执行整车控制任务：解析遥控器命令，生成各模块控制命令
      RoboCmdTask();
    }
    // 延迟5ms，让出CPU给其他任务，控制任务周期为5ms
    // 使用vTaskDelay而不是osDelay，因为这是FreeRTOS原生API
    vTaskDelay(5);
  }
}

/**
 * @brief  底盘任务实现函数
 * @param  argument: 任务参数（未使用）
 * @retval None
 * 
 * 功能说明：
 * 1. 检查遥控器数据是否准备好（Rc_Start标志）
 * 2. 如果数据准备好，顺序执行三个模块的控制：
 *    - GimbalTask(): 云台控制（Yaw和Pitch轴）
 *    - Chassistask(): 底盘运动控制（麦轮运动学解算）
 *    - ShootTask(): 射击控制（摩擦轮、拨弹盘）
 * 3. 所有模块执行完后，统一调用MotorControl()计算PID并发送CAN报文
 * 4. 每5ms执行一次，保证控制频率为200Hz
 * 
 * 设计说明：
 * - 将三个模块的控制放在一个任务中，保证控制的同步性
 * - 在同一周期内完成所有计算，避免时间差导致的控制不同步
 * - 优先级为Idle，与其他控制任务平等竞争CPU
 */
void ChassisTask(void const *argument)
{
  while(1)  // 任务必须包含无限循环
  {
    // 检查遥控器数据接收标志
    // 只有接收到遥控器数据后，才执行控制任务
    if (Rc_Start == 1)
    {
        // 顺序执行三个模块的控制任务
        // 注意：执行顺序很重要，因为可能存在数据依赖关系
        
        GimbalTask();    // 云台控制：计算Yaw和Pitch轴目标角度，设置电机参考值
        Chassistask();   // 底盘控制：运动学解算，计算四个轮子速度，设置电机参考值
        ShootTask();     // 射击控制：根据射击模式设置摩擦轮和拨弹盘速度
        
        // 注意：MotorControl()在Chassistask()内部调用，统一处理所有电机
        // 这样设计可以批量发送CAN报文，减少总线负载
    }
    
    // 延迟5ms，控制任务周期为5ms（200Hz控制频率）
    vTaskDelay(5);
  }
}

/**
 * @brief  视觉任务实现函数
 * @param  argument: 任务参数（未使用）
 * @retval None
 * 
 * 功能说明：
 * 1. 调用WatchingTra()发送数据给视觉模块
 * 2. WatchingTra()会：
 *    - 打包云台姿态数据（Yaw、Pitch角度）
 *    - 通过USART6发送16字节数据包
 * 3. 每100ms执行一次（10Hz）
 * 
 * 设计说明：
 * - 视觉通信频率不需要太高，10Hz足够
 * - 优先级设为High，确保视觉数据及时发送
 * - 接收数据处理在main.c的UART IDLE中断中完成（WatchingRec()）
 * 
 * 通信协议：
 * - 发送：0xFF + 颜色(1) + Pitch(4) + Yaw(4) + 保留(6) + 0xFE
 * - 接收：0xFF + 开火命令(1) + Yaw目标(4) + Pitch目标(4) + 距离(4) + 0xFE
 */
void WatchingTask(void const *argument)
{
  while(1)  // 任务必须包含无限循环
  {
    // 发送视觉数据：云台姿态角度（Yaw、Pitch）
    // 视觉模块根据这些数据计算目标角度并返回
    WatchingTra();
    
    // 延迟100ms，视觉通信周期为100ms（10Hz）
    // 这个频率足够视觉模块进行目标识别和跟踪
    vTaskDelay(100);
  }
}
/* USER CODE END Application */
