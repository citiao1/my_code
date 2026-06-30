#include "ultrasonic.h"
#include "gpio.h"
#include "tim.h"
#include "filter.h"
#include "main.h"
#define FILTER_SIZE 5
#define MAX_STEP 20  //超声波变化最大步长

float D_front = 0;
float D_left  = 0;
float D_right = 0;

typedef enum {
    US_IDLE,
    US_TRIG_HIGH,
    US_WAIT_ECHO
} US_State_t;
/*
typedef struct {
    GPIO_TypeDef* trig_port;
    uint16_t      trig_pin;
    uint32_t      tim_channel;      // 定时器通道宏 (用于设置极性)
    uint32_t      active_channel;   // 活跃通道宏 (用于中断判断)
    
    uint32_t      up_edge;          // 记录上升沿时间
    uint32_t      down_edge;        // 记录下降沿时间
    uint8_t       capture_flag;     // 状态机：0-等上升沿, 1-等下降沿
    
    float         distance;         // 本次测算距离
} Ultrasonic_t;
static Ultrasonic_t us_sensors[3] = {
    {GPIOA, GPIO_PIN_6, TIM_CHANNEL_1, HAL_TIM_ACTIVE_CHANNEL_1, 0, 0, 0, 0}, // Front
    {GPIOA, GPIO_PIN_7, TIM_CHANNEL_2, HAL_TIM_ACTIVE_CHANNEL_2, 0, 0, 0, 0}, // Left
    {GPIOB, GPIO_PIN_0, TIM_CHANNEL_3, HAL_TIM_ACTIVE_CHANNEL_3, 0, 0, 0, 0}  // Right
};


void Ultrasonic_Poll(void) {
    static uint32_t last_time = 0;
    static uint8_t current_sensor = 0; // 当前正在触发的传感器编号
    
    // 每 20ms 触发一个传感器，交替进行，彻底杜绝声波串扰
    if(HAL_GetTick() - last_time > 20) {
        last_time = HAL_GetTick();
        
        // 1. 给当前传感器发送至少 10us 的高电平触发脉冲
        HAL_GPIO_WritePin(us_sensors[current_sensor].trig_port, us_sensors[current_sensor].trig_pin, GPIO_PIN_SET);
        // 简单延时十几微秒 (168MHz / 72MHz 皆适用)
        for(volatile int i = 0; i < 150; i++); 
        HAL_GPIO_WritePin(us_sensors[current_sensor].trig_port, us_sensors[current_sensor].trig_pin, GPIO_PIN_RESET);
        
        // 2. 指针切换到下一个传感器
        current_sensor++;
        if(current_sensor >= 3) {
            current_sensor = 0;
        }
    }
}
*/
static US_State_t us_state = US_IDLE;

static uint32_t us_time = 0;

static float dist_buf[FILTER_SIZE] = {0};
static int buf_index = 0;

static float dist_last = 0;   // 上一次值（限幅用）
static float dist_lp = 0;     // 低通输出



void Ultrasonic_Trig(void)
{
    switch(us_state)
    {
    case US_IDLE:
        // 每50ms触发一次
        if (HAL_GetTick() - us_time > 50)
        {
            HAL_GPIO_WritePin(TRIG1_GPIO_Port, TRIG1_Pin, GPIO_PIN_SET);
            us_time = HAL_GetTick();
            us_state = US_TRIG_HIGH;
        }
        break;

    case US_TRIG_HIGH:
        // 保持10us（这里用1ms也能工作，HC-SR04不严格）
        if (HAL_GetTick() - us_time > 1)
        {
            HAL_GPIO_WritePin(TRIG1_GPIO_Port, TRIG1_Pin, GPIO_PIN_RESET);
            us_state = US_WAIT_ECHO;
        }
        break;

    case US_WAIT_ECHO:
        // 等输入捕获中断更新distance
        // 这里什么都不用做
        us_state = US_IDLE;
        break;
    }
}

float Ultrasonic_GetDistance_NonBlocking(void)
{
    if(distance <= 0 || distance > 400)
        return 400;
    return distance;
}

float Ultrasonic_GetDistance_Safe(void)
{
    if (HAL_GetTick() - g_distance_time > 50)  // 100ms没更新
    {
				HAL_TIM_IC_Stop_IT(&htim3,TIM_CHANNEL_1);
				__HAL_TIM_SET_COUNTER(&htim3, 0);
				HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
        return 400;  // 当作“无限远”
    }

    return g_distance;
}

float Ultrasonic_GetDistance_Filtered(void)
{
    static float buf[3] = {400, 400, 400};
    static uint8_t index = 0;
		static uint8_t last_index = 0;

    float val;
		
    // 超时处理（必须保留）
    if(HAL_GetTick() - g_distance_time > 100)
    {
        val = 400;
    }
    else if(buf[last_index] + MAX_STEP<g_distance){
	    val = buf[last_index] + MAX_STEP;
	}
    else if(buf[last_index] - MAX_STEP > g_distance){
		val = buf[last_index] - MAX_STEP;
	}
    else{
        val = g_distance;
    }
		

    // 存入缓冲区
		last_index = index;
    buf[index++] = val;
    if(index >= 3) index = 0;

    // 求平均
    float sum = 0;
    for(int i = 0; i < 3; i++)
        sum += buf[i];

    return sum / 3.0f;
}
