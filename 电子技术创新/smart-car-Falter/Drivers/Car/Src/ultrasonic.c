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


/**
 超声波传感器轮询触发函数
 该函数通过状态机机制轮流触发三个超声波传感器，避免同时触发造成的信号干扰。
 1. 根据当前激活的传感器ID，获取上一个传感器的滤波后距离值。
 2. 检查时间间隔，确保每个传感器有足够的测量周期（约40ms间隔）。
 3. 满足时间条件后，触发下一个传感器的Trig引脚，并更新状态机ID。
 4. 完成一轮循环（触发完第3个传感器）后，重置基准时间 us_time。
 需在主循环或定时器中断中周期性调用此函数。
 */
void Ultrasonic_Trig(void)
{
    switch(Activ_Ultrasonic){
			case Ultrasonic_1:
				// 获取超声波3的滤波距离（上一轮测量的结果）
				dist3 = Ultrasonic3_GetDistance_Filtered();
				// 检查是否经过至少40ms，防止触发频率过高
				if(HAL_GetTick()-us_time >= 40){
					// 触发超声波1
					Ultrasonic_Trig_Pin(TRIG1_GPIO_Port, TRIG1_Pin);
					// 切换到下一个状态：准备触发超声波2
					Activ_Ultrasonic = Ultrasonic_2;
				}
				
				break;
			case Ultrasonic_2:
				// 获取超声波1的滤波距离
				dist1 = Ultrasonic1_GetDistance_Filtered();
				// 检查是否经过至少80ms（相对于起始时间）
				if(HAL_GetTick()-us_time >= 80){
					// 触发超声波2
					Ultrasonic_Trig_Pin(TRIG2_GPIO_Port, TRIG2_Pin);
					// 切换到下一个状态：准备触发超声波3
					Activ_Ultrasonic = Ultrasonic_3;
				}
				break;
			case Ultrasonic_3:
				// 获取超声波2的滤波距离
				dist2 = Ultrasonic2_GetDistance_Filtered();
				// 检查是否经过至少120ms（相对于起始时间），完成一轮循环
				if(HAL_GetTick()-us_time >= 120){
					// 触发超声波3
					Ultrasonic_Trig_Pin(TRIG3_GPIO_Port, TRIG3_Pin);
					// 切换回初始状态：准备下一轮触发超声波1
					Activ_Ultrasonic = Ultrasonic_1;
					// 重置基准时间，开始新的计时周期
					us_time = HAL_GetTick();
				}
				break;
		}
		
}

/**
 产生超声波模块的Trig触发脉冲
 GPIOx GPIO端口
 Trig_PIN 触发引脚
 拉高引脚 -> 延时10us -> 拉低引脚，产生一个宽度为10us的高电平脉冲，
 用于启动超声波模块进行测距。
 */
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

/**
 获取超声波1经过滤波后的距离值
 采用滑动窗口平均滤波算法，包含以下处理机制：
 1. 有效性检查：仅处理0-400cm范围内的有效数据
 2. 超时处理：若超过100ms未更新数据，视为无效，返回最大距离400cm
 3. 阶跃限制：单次变化不超过MAX_STEP(20cm)，防止突变干扰
 4. 均值滤波：对最近3次有效处理后的数据求平均值
 滤波后的距离值 (cm)
 */
float Ultrasonic1_GetDistance_Filtered(void)
{
    static float buf1[3] = {400, 400, 400}; // 滑动窗口缓冲区，初始化为最大距离
    static uint8_t index1 = 0;              // 当前写入索引
    static uint8_t last_index1 = 0;         // 上一次写入索引，用于阶跃判断基准

    float val;
		
    // 仅允许合法范围内的值参与滤波逻辑
    if (distance1 > 0.0f && distance1 <= 400.0f)
    {
        // 超时处理：若距离数据超过100ms未更新，强制返回最大距离
        if(HAL_GetTick() - g_distance_time_1 > 100)
            val = 400;
        // 阶跃限制：防止数据突然增大，限制最大增长步长
        else if(buf1[last_index1] + MAX_STEP < distance1)
        {
            val = buf1[last_index1] + MAX_STEP;
        }
        // 阶跃限制：防止数据突然减小，限制最大减小步长
        else if(buf1[last_index1] - MAX_STEP > distance1)
        {
            val = buf1[last_index1] - MAX_STEP;
        }
        // 正常情况：直接使用当前测量值
        else
        {
            val = distance1;
        }
			
        // 将处理后的值存入缓冲区
        last_index1 = index1;       // 更新上一次索引
        buf1[index1++] = val;       // 存入当前值并移动索引
        if(index1 >= 3) index1 = 0; // 环形缓冲区索引复位
		}
    
    // 计算缓冲区数据的平均值
    float sum = 0;
    for(int i = 0; i < 3; i++)
        sum += buf1[i];

    return sum / 3.0f;
}

/**
 获取超声波2经过滤波后的距离值
 逻辑同Ultrasonic1_GetDistance_Filtered，针对超声波2传感器
 滤波后的距离值 (cm)
 */
float Ultrasonic2_GetDistance_Filtered(void)
{
    static float buf2[3] = {400, 400, 400};
    static uint8_t index2 = 0;
    static uint8_t last_index2 = 0;

    float val;
		
    // 超时处理及有效性检查
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

/**
 获取超声波3经过滤波后的距离值
 逻辑同Ultrasonic1_GetDistance_Filtered，针对超声波3传感器
 滤波后的距离值 (cm)
 */
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