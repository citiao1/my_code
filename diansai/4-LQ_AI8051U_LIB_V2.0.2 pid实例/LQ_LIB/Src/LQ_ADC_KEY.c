/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 40 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#include "LQ_ADC_KEY.h"
#include "include.h"

/*************************************************************************
 *  函数名称：void DMA_ADKey_Init()
 *  功能说明：ADC按键DMA初始化函数
 *  参数说明：无
 *  函数返回：无
 *  修改时间：2026年1月12日
 *  备    注：建议用户将 ADKey_Scan()函数放到定时器或主循环定时运行,保证实时性
 *   ADC_CH10_P02 为ADC按键 -SW5
 *   ADC_CH11_P03 为ADC旋钮
 *************************************************************************/
void ADC_Key_Init()
{
    ADC_InitTypeDef ADC_InitStructure; // 结构定义

    ADC_GPIO_Init(ADC_CH10_P02); // P02_ADC五向按键  ADC配置GPIO，模式
    gpio_speed(P0_2, ENABLE);    // 使能高速
    ADC_GPIO_Init(ADC_CH11_P03);

    ADC_InitStructure.ADC_SMPduty = 31;                    // ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10
    ADC_InitStructure.ADC_CsSetup = 0;                     // ADC 通道选择时间控制 0(默认),1
    ADC_InitStructure.ADC_CsHold = 1;                      // ADC 通道选择保持时间控制 0,1(默认),2,3
    ADC_InitStructure.ADC_Speed = ADC_SPEED_2X16T;         // 设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
    ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED; // ADC结果d对齐方式,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
    ADC_Inilize(&ADC_InitStructure);                       // 初始化
    ADC_PowerControl(ENABLE);                              // ADC电源开关, ENABLE或DISABLE
    NVIC_ADC_Init(DISABLE, Priority_0);                    // 中断使能, ENABLE/DISABLE; 优先级(低到高)--> Priority_0~Priority_3
}

/*************************************************************************
 *  函数名称：u8 Get_ADC_Key()
 *  功能说明：传入ADC 按键通道的ADC值，返回ADC按键键值
 *  参数说明：u16 key_ADval：ADC值
 *  函数返回：ADC按键的键值
 *  修改时间：2026年1月12日
 *  备    注：无
 *************************************************************************/
u8 Get_ADC_Key()
{
    u16 key_ADval = Get_ADCResult(ADC_CH10_P02); // Get_DmaAD_val();
    // 根据ADC值范围判断按键,粗略计算
    // if(key_ADval < 500)        return KEY_PRESS;     // K0≈0
    // else if(key_ADval < 1500) return KEY_UP;        // Ku≈1028
    // else if(key_ADval < 2700) return KEY_RIGHT;     // Kr≈2345
    // else if(key_ADval < 3400) return KEY_DOWN;      // Kd≈3110
    // else if(key_ADval < 3900) return KEY_LEFT;      // Kl≈3630

    if (key_ADval < 3850 && key_ADval > 3500)        return KEY_LEFT;  // Kl≈3630
    else if (key_ADval < 3400 && key_ADval > 2900)   return KEY_DOWN;  // Kd≈3110
    else if (key_ADval < 2700 && key_ADval > 2000)   return KEY_RIGHT; // Kr≈2345
    else if (key_ADval < 1500 && key_ADval > 800)    return KEY_UP;    // Ku≈1028
    else if (key_ADval < 500)                        return KEY_PRESS; // K0≈0
    else                                             return KEY_NONE;  // 无按键≈4096
}

/*************************************************************************
 *  函数名称：u8 ADKey_Scan_Task(void)
 *  功能说明：ADC按键扫描任务函数
 *  参数说明：无
 *  函数返回：按键状态
 *  修改时间：2026年1月10日
 *  备    注：建议用户将本函数放到定时器中断或主循环定时运行，
 *  如果循环时间不足，可以直接使用 Get_ADC_Key(); // 得到ADC对应的键值
 *************************************************************************/
u8 ADKey_Scan(void)
{
    static u8 last_key = KEY_NONE;
    static u8 stable_count = 0;

    u8 current_key = Get_ADC_Key(); // 得到ADC对应的键值
    if (current_key == last_key)
    {
        stable_count++;
        if (stable_count >= 3)
        { // 连续3次检测相同认为稳定
            stable_count = 0;
            return current_key;
        }
    }
    else
    {
        stable_count = 0;
        last_key = current_key;
    }

    return KEY_NONE;
}



