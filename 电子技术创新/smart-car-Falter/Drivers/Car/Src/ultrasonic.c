#include "ultrasonic.h"
#include "gpio.h"
#include "tim.h"
#include "filter.h"
#define FILTER_SIZE 5
#define MAX_STEP 20  //超声波变化最大步长

typedef enum {
  Ultrasonic_1,
	Ultrasonic_2,
	Ultrasonic_3,
}Ultrasonic_ID;
	
static Ultrasonic_ID Activ_Ultrasonic = Ultrasonic_1;
static uint32_t us_time = 0;

static float dist_buf[FILTER_SIZE] = {0};
static int buf_index = 0;

static float dist_last = 0;   // 上一次值（限幅用）
static float dist_lp = 0;     // 低通输出



void Ultrasonic_Trig(void)
{
    switch(Activ_Ultrasonic){
			case Ultrasonic_1:
				dist3 = Ultrasonic3_GetDistance_Filtered();
				if(HAL_GetTick()-us_time >= 40){
					Ultrasonic_Trig_Pin(TRIG1_GPIO_Port, TRIG1_Pin);
					Activ_Ultrasonic = Ultrasonic_2;
				}
				
				break;
			case Ultrasonic_2:
				dist1 = Ultrasonic1_GetDistance_Filtered();
				if(HAL_GetTick()-us_time >= 80){
					Ultrasonic_Trig_Pin(TRIG2_GPIO_Port, TRIG2_Pin);
					Activ_Ultrasonic = Ultrasonic_3;
				}
				break;
			case Ultrasonic_3:
				dist2 = Ultrasonic2_GetDistance_Filtered();
				if(HAL_GetTick()-us_time >= 120){
					Ultrasonic_Trig_Pin(TRIG3_GPIO_Port, TRIG3_Pin);
					Activ_Ultrasonic = Ultrasonic_1;
					us_time = HAL_GetTick();
				}
				break;
		}
		
}

void Ultrasonic_Trig_Pin(GPIO_TypeDef* GPIOx ,uint16_t Trig_PIN){
	HAL_GPIO_WritePin(GPIOx, Trig_PIN, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(GPIOx, Trig_PIN, GPIO_PIN_RESET);
}


/*float Ultrasonic_GetDistance_NonBlocking(void)
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
*/

float Ultrasonic1_GetDistance_Filtered(void)
{
    static float buf1[3] = {400, 400, 400};
    static uint8_t index1 = 0;
    static uint8_t last_index1 = 0;

    float val;
		
    // 超时处理（必须保留）
   if (distance1 > 0.0f && distance1 <= 400.0f)
    {
        if(HAL_GetTick() - g_distance_time_1 > 100)
            val = 400;
        else if(buf1[last_index1] + MAX_STEP < distance1)
        {
            val = buf1[last_index1] + MAX_STEP;
        }
        else if(buf1[last_index1] - MAX_STEP > distance1)
        {
            val = buf1[last_index1] - MAX_STEP;
        }
        else
        {
            val = distance1;
        }
			

    // 存入缓冲区
    last_index1 = index1;
    buf1[index1++] = val;
    if(index1 >= 3) index1 = 0;
		}
    // 求平均
    float sum = 0;
    for(int i = 0; i < 3; i++)
        sum += buf1[i];

    
	
		return sum / 3.0f;
}
float Ultrasonic2_GetDistance_Filtered(void)
{
    static float buf2[3] = {400, 400, 400};
    static uint8_t index2 = 0;
    static uint8_t last_index2 = 0;

    float val;
		
    // 超时处理（必须保留）
    if (distance2 > 0.0f && distance2 <= 400.0f)
    {
        if(HAL_GetTick() - g_distance_time_2 > 100)
            val = 400;
        else if(buf2[last_index2] + MAX_STEP < distance2)
        {
            val = buf2[last_index2] + MAX_STEP;
        }
        else if(buf2[last_index2] - MAX_STEP > distance2)
        {
            val = buf2[last_index2] - MAX_STEP;
        }
        else
        {
            val = distance2;
        }
        
        last_index2 = index2;
        buf2[index2++] = val;
        if(index2 >= 3) index2 = 0;
    }
    // 求平均
    float sum = 0;
    for(int i = 0; i < 3; i++)
        sum += buf2[i];

    return sum / 3.0f;
}

float Ultrasonic3_GetDistance_Filtered(void)
{
    static float buf3[3] = {400, 400, 400};
    static uint8_t index3 = 0;
    static uint8_t last_index3 = 0;

    float val;
		
    if (distance3 > 0.0f && distance3 <= 400.0f)
    {
        if(HAL_GetTick() - g_distance_time_3 > 100)
            val = 400;
        else if(buf3[last_index3] + MAX_STEP < distance3)
        {
            val = buf3[last_index3] + MAX_STEP;
        }
        else if(buf3[last_index3] - MAX_STEP > distance3)
        {
            val = buf3[last_index3] - MAX_STEP;
        }
        else
        {
            val = distance3;
        }
        
        last_index3 = index3;
        buf3[index3++] = val;
        if(index3 >= 3) index3 = 0;
    }

    // 求平均
    float sum = 0;
    for(int i = 0; i < 3; i++)
        sum += buf3[i];

    return sum / 3.0f;
}