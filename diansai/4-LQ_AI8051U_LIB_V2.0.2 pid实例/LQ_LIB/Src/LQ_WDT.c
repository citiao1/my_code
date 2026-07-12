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

#include "include.h"

/**************************************************************************************
 * 函数: WDT_Init
 * 描述: 用户初始化程序.
 * 参数: WDT_SCALE_s 0到7
 * 返回: None.
 * 版本: V1.0, 2024-10-28
 * 时间计算： WDT_Timeout=(12*32768*SCALE)/SYSclk
 *      其中SCALE，给出8个可选值，为 2^(1+WDT_SCALE_s)
 *      若WDT_SCALE_256，则WDT_Timeout=12*32768*256/35000000=2.876094171428571  约2.88s

 *========================================================================
 * WDT_PS：看门狗定时器时钟分频系数一下对应0到7
 *      分频系数参数    分频系数   45158400MHz下时间       其他频率下的时间自根据公式计算
 *      WDT_SCALE_2,       0         约0.017 s
 *      WDT_SCALE_4,       1         约0.035 s
 *      WDT_SCALE_8,       2         约0.070 s
 *      WDT_SCALE_16,      3         约0.139 s
 *      WDT_SCALE_32,      4         约0.279 s
 *      WDT_SCALE_64,      5         约0.557 s
 *      WDT_SCALE_128,     6         约1.115 s
 *      WDT_SCALE_256      7         约2.229 s
 ***************************************************************************************/
void WDT_Init(WDT_PS_SCALE WDT_SCALE_s)
{
	WDT_InitTypeDef	WDT_InitStructure;					//看门狗 结构定义

	WDT_InitStructure.WDT_Enable     = ENABLE;			//中断使能   ENABLE或DISABLE
	WDT_InitStructure.WDT_IDLE_Mode  = WDT_IDLE_STOP;	//IDLE模式是否停止计数		WDT_IDLE_STOP,WDT_IDLE_RUN
	WDT_InitStructure.WDT_PS         = WDT_SCALE_s;	    //看门狗定时器时钟分频系数 0~7
	WDT_Inilize(&WDT_InitStructure);					//初始化

	RSTFLAG |= 0x04;   //复位寄存器RSTFLAG，设置看门狗复位需要检测P3.2的状态，否则看门狗复位后可能进入USB下载模式
}

