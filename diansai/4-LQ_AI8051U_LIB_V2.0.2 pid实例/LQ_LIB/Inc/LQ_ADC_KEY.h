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

#ifndef __LQ_ADC_KEY_H_
#define __LQ_ADC_KEY_H_

#include "config.h"


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

void ADC_Key_Init(void);  // ADC初始化
u8 Get_ADC_Key(void);        // 解析ADC按键键值
u8 ADKey_Scan(void);         // ADC按键扫描，建议方定时器中断（推荐<=50ms）或主循环


#endif



