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

#include "include.h"
#include "LQ_ADC.h"

/***********************************************************************************
 * 函 数 名：void ADC_Init(void)
 * 功    能：ADC初始化，管脚初始化和中断模式、使能等设置
 * 参    数：无
 * 返 回 值：无
 * 说    明：    //ADC 端口IO配置，详见ADC_CHx_Pin
 **********************************************************************************/
void ADC_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure; // 结构定义

//    ADC_GPIO_Init(ADC_CH8_P00); // ADC 端口IO配置，要初始化的ADC通道和管脚，详见ADC_CHx_Pin
    ADC_GPIO_Init(ADC_CH9_P01); //  mini驱控一体板的 电源ADC检测，
    ADC_GPIO_Init(ADC_CH0_P10);
    ADC_GPIO_Init(ADC_CH1_P11);
    ADC_GPIO_Init(ADC_CH2_P12);
    ADC_GPIO_Init(ADC_CH3_P13);
    ADC_GPIO_Init(ADC_CH4_P14);
	  ADC_GPIO_Init(ADC_CH5_P15);

    // ADC配置
    ADC_InitStructure.ADC_SMPduty = 31;                    // ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
    ADC_InitStructure.ADC_CsSetup = 0;                     // ADC 通道选择时间控制 0(默认),1
    ADC_InitStructure.ADC_CsHold = 1;                      // ADC 通道选择保持时间控制 0,1(默认),2,3
    ADC_InitStructure.ADC_Speed = ADC_SPEED_2X16T;         // 设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
    ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED; // ADC结果d对齐方式,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
    ADC_Inilize(&ADC_InitStructure);                       // 初始化
    ADC_PowerControl(ENABLE);                              // ADC电源开关, ENABLE或DISABLE
    NVIC_ADC_Init(DISABLE, Priority_0);                    // 断使能, ENABLE/DISABLE; 优先级(低到高)--> Priority_0~Priority_3
}

/**
 * @brief ADC测试函数
 * 该函数用于初始化ADC和OLED显示屏，并在OLED上显示多个ADC通道的采样值
 */

//#define USE_OLED  // 使用USE_OLED
#define USE_IPS_LCD  // 使用IPS_LCD

void Test_ADC()
{
    char txt[20];        // 用于存储格式化字符串的字符数组
    ADC_Init();          // ADC初始化, 管脚初始化和中断模式、使能等设置
    Global_IRQ_Enable(); // 被注释的全局中断使能函数
#ifdef USE_OLED
    OLED_Init();                              // OLED初始化
    OLED_CLS();                               // 加入OLED显示
    OLED_P6x8Str(10, 0, "LQ ADC_All Test");   // 在OLED上显示标题
#elif defined(USE_IPS_LCD)
    IPS_LCD_Init();        //LCD初始化  0:横屏  1：竖屏
		LCD_CLS(u16_BLACK);
    LCD_ShowStr_16(0,0,"LQ ADC_All Test",u16_RED_IPS,u16_BLACK);     //显示汉字16*8
#endif  
    printf("AD to UART Test Programme!\r\n"); // UART发送一个字符串，表示程序开始

    while (1)
    {
#ifdef USE_OLED
//        sprintf(txt, "ADV:%05d ", Get_ADCResult(ADC_CH8_P00)); // ADC_CH8-P00做电源电压 11.0V->1700左右，12.6V->1970左右
        sprintf(txt, "ADV:%05d ", Get_ADCResult(ADC_CH10_P02)); // ADC_CH8-P00做电源电压 11.0V->1700左右，12.6V->1970左右
        printf(txt);
        OLED_P6x8Str(10, 1, txt);                              // 显示电源电压值
        sprintf(txt, "L:%05d ", Get_ADCResult(ADC_CH0_P15)); // ADC0-P10
        printf(txt);
        OLED_P6x8Str(10, 2, txt);                              // 显示
        sprintf(txt, "L1:%05d ", Get_ADCResult(ADC_CH1_P11)); // ADC1P11
        OLED_P6x8Str(10, 3, txt);                              // 显示
        sprintf(txt, "M:%05d ", Get_ADCResult(ADC_CH2_P12)); // ADC2-P12
        OLED_P6x8Str(10, 4, txt);                              // 显示通道5的ADC值
        sprintf(txt, "R:%05d ", Get_ADCResult(ADC_CH3_P13)); // ADC3-P13
        OLED_P6x8Str(10, 5, txt);                              // 显示通道7的ADC值
        sprintf(txt, "R1:%05d ", Get_ADCResult(ADC_CH4_P14)); // ADC4-P14
        OLED_P6x8Str(10, 6, txt);                              // 显示通道7的ADC值
#elif defined(USE_IPS_LCD)
        sprintf(txt, "ADV:%05d ", Get_ADCResult(ADC_CH10_P02)); // ADC_CH8-P00做电源电压 11.0V->1700左右，12.6V->1970左右
        printf(txt);
        LCD_ShowStr_16(0,1,txt,u16_RED_IPS,u16_BLACK);         // 显示电源电压值
        sprintf(txt, "L1:%05d ", Get_ADCResult(ADC_CH5_P15)); // ADC5-P15
        printf(txt);
        LCD_ShowStr_16(0,2,txt,u16_RED_IPS,u16_BLACK); 
        sprintf(txt, "L2:%05d ", Get_ADCResult(ADC_CH1_P11)); // ADC1P11
        LCD_ShowStr_16(0,3,txt,u16_RED_IPS,u16_BLACK);      
        sprintf(txt, "M:%05d ", Get_ADCResult(ADC_CH2_P12)); // ADC2-P12
        LCD_ShowStr_16(0,4,txt,u16_RED_IPS,u16_BLACK);                     // 显示通道5的ADC值
        sprintf(txt, "R1:%05d ", Get_ADCResult(ADC_CH3_P13)); // ADC3-P13
        LCD_ShowStr_16(0,5,txt,u16_RED_IPS,u16_BLACK);                    // 显示通道7的ADC值
        sprintf(txt, "R2:%05d ", Get_ADCResult(ADC_CH4_P14)); // ADC4-P14
        LCD_ShowStr_16(0,6,txt,u16_RED_IPS,u16_BLACK);                              // 显示通道7的ADC值
        
#endif       
        delay_ms(100);                                         // 延时100ms，控制刷新频率
    }
}

