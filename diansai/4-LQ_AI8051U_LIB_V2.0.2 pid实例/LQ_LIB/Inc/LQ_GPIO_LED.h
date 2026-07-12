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

#ifndef __BEEP_LED_H_
#define __BEEP_LED_H_

#include "include.h"

#define BEEP_PIN            P3_5                                /* 蜂鸣器管脚选择宏定义 */

#define BEEP_ON()           gpio_write_pin(BEEP_PIN,HIGH)       /* 打开蜂鸣器 */
#define BEEP_OFF()          gpio_write_pin(BEEP_PIN,LOW)        /* 关闭蜂鸣器 */
#define BEEP_Toggle()       gpio_toggle_pin(BEEP_PIN)           /* 翻转蜂鸣器状态 */
#define gpio_beep_init()    gpio_init_pin(BEEP_PIN,GPIO_Mode_Out_PP),BEEP_OFF()
// =================================================================================

#define LED_PIN0            P01                                 /* LED管脚选择宏定义 */
#define LED_PIN1            P27
#define LED_PIN2            P01
/* 这样操作最直接省时,修改时上下保持一致 */
#define LED_0P              P0_1                                /* LED管脚初始化选择宏定义 */
#define LED_1P              P2_7
#define LED_2P              P0_1

typedef enum
{
  LED0 = 0,
  LED1 = 1,
  LED2 = 2,
  LED_ALL,
}LED_n;

//枚举LED状态
typedef enum
{
  ON  = 0,
  OFF = 1,
  RVS = 2,
}LED_s;

//-------------------------用户函数声明-------------------------

void GPIO_LED_Init(void);
void LED_Ctrl(LED_n ledp, LED_s sta);


#endif  /*__BEEP_LED_H_*/
