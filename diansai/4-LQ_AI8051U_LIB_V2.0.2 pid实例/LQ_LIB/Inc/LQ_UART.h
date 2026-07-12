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
#ifndef  __LQ_UART_H
#define  __LQ_UART_H

#include "config.h"
#include "AI8051U_GPIO.h"
typedef enum
{
    UART1_P30_P31 = 0x0000|P3_0, // 31，只需要结果加1,
    UART1_P36_P37 = 0x0100|P3_6,
    UART1_P16_P17 = 0x0200|P1_6,
    UART1_P43_P44 = 0x0300|P4_3,

    UART2_P12_P13 = 0x1000|P1_2,
    UART2_P42_P43 = 0x1100|P4_2,

    UART3_P00_P01 = 0x2000|P0_0,
    UART3_P50_P51 = 0x2100|P5_0,

    UART4_P02_P03 = 0x3000|P0_2,
    UART4_P52_P53 = 0x3100|P5_2,
}UARTn_RxTx_Pin;


void UART_Init(UARTn_RxTx_Pin UARTx_Pin,u32 BaudRate);

void Test_UART_DMA();


#endif

