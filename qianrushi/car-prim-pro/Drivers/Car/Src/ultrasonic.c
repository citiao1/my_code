#include "ultrasonic.h"
#include "gpio.h"
#include "tim.h"
#include "filter.h"
#define FILTER_SIZE 5
#define MAX_STEP 20

typedef enum {
    US_IDLE,
    US_TRIG_HIGH,
    US_WAIT_ECHO
} US_State_t;

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
            HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
            us_time = HAL_GetTick();
            us_state = US_TRIG_HIGH;
        }
        break;

    case US_TRIG_HIGH:
        // 保持10us（这里用1ms也能工作，HC-SR04不严格）
        if (HAL_GetTick() - us_time > 1)
        {
            HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
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
        val = 400;
    else if(buf[last_index] + MAX_STEP<g_distance)
		{
				val = buf[last_index] + MAX_STEP;
		}else if(buf[last_index] - MAX_STEP > g_distance)
		{
				val = buf[last_index] - MAX_STEP;
		}else{
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
