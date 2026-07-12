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
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
#ifndef __LQ_XINT_H
#define __LQ_XINT_H

#include "config.h"


// GPIO中断模式枚举定义
typedef enum
{
	PxINT_MODE_Fall = 0,	//下降沿中断
	PxINT_MODE_Rise,    	//上升沿中断
	PxINT_MODE_LOW,     	//低电平中断
	PxINT_MODE_HIGH	    	//高电平中断
} GPIO_INT_Mode_e;


// ===============================================================================================
// GPIO中断初始化函数
u8 GPIO_INT_Config(u8 GPIO, GPIO_InitTypeDef *GPIOx);  /* GPIO_PIN中断初始化函数,内部调用，用户无需关心 */

//IO中断用户函数
void GPIO_PxINT_Init(Pin_list Pinx, GPIO_INT_Mode_e INT_Mode,u8 Priority_n); /* GPIO_PIN中断用户初始化函数 */

// ===============================================================================================
// 外部中断用户函数
void Exti_Init(u8 EXT_INTx, u8 Priority_n);

#endif

