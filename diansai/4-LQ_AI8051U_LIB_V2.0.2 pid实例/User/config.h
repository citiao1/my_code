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
 【SYS CLK】 MAIN_Fosc 使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#ifndef	 __TYPE_DEF_H
#define	 __TYPE_DEF_H

#include <stdlib.h>
#include <DEF.h>
#include "STC_AI8051U_X.h"


//宏可设置主时钟频率
#define MAIN_Fosc		40000000L	//定义主时钟频率     //使用40M 作为系统主频



//设置系统等待时间单元，用于控制EEPROM操作、SPI/I2C超时时间以及休眠唤醒等待时间
#define SET_TPS()       IAP_TPS = (MAIN_Fosc / 1000000L)
//设置IRC时钟从休眠唤醒恢复稳定需要等待的时钟数，初始值: 0x80，高频建议设置: 0x10
#define IRC_Debounce(n) IRCDB = n
//========================================================================
//                               类型定义
//========================================================================


#define NULLD               0    //类型不一样,避免标准库中的NULL冲突
#define SUCCESS		        0
#define FAIL		        -1


//全局中断使能/失能设置
#define Global_IRQ_Enable()    (EA = 1)
#define Global_IRQ_Disable()   (EA = 0)

#endif
