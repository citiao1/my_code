#ifndef __LQ_ADC_KEY_H_
#define __LQ_ADC_KEY_H_

#include "lq_include.h"

/* 选择 按键和旋钮的ADC通道 */
#define AD_knob    ADC7   // ADC旋钮通道号
#define AD_KEY_CH  ADC9   // ADC按键通道

#define AN3_KEY    ADC3   // 一体板上的AN3按键
#define M_KEY      P33_9  // 一体板上的339按键


/* MCU 接口配置 */
#define ADK_UART_PORT       (UART1)             // 配置串口
#define ADK_UART_RX_PIN     (UART1_RX_P15_5)    // 配置串口引脚
#define ADK_UART_TX_PIN     (UART1_TX_P15_5)
#define ADK_UART_BAUD       (115200ul)          // 配置串口波特率

//定义模块号
typedef enum
{
    KEY_NONE = 0,  /* 没有按键按下 */
    KEY_UP,        /* 向上触发 */
    KEY_DOWN,	   /* 向下触发 */
    KEY_LEFT,	   /* 向左触发 */
    KEY_RIGHT,     /* 向右触发 */
    KEY_PRESS      /* 按下触发 */
}ADC_KEY_Name;


// =============================== 外部函数声明 =================================

void ADC_Key_Init(void);        // ADC初始化
uint8 Get_ADC_Key(void);        // 解析ADC按键键值
uint8 ADKey_Scan(void);         // ADC按键扫描，建议方定时器中断（推荐<=50ms）或主循环
void Test_ADC_Key(void);

#endif



