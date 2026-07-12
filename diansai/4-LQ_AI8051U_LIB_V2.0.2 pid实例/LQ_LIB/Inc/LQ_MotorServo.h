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

#ifndef __LQ_MS_H_
#define __LQ_MS_H_

#include "include.h"


#define Motor_Dir1        P5_1         //电机驱动方向控制IO1,宏定义
#define Motor_Dir2        P5_3         //电机驱动方向控制IO2,宏定义

#define M1_DIR_PIN1       P51           /* 用于直接操作寄存器，需与上面保持一致 */
#define M1_DIR_PIN2       P53

#define USE_DRV8701                   //使用不同的驱动，定义不同的宏定义
// #define USE_MIT7971                   //使用不同的驱动，定义不同的宏定义

#define Motor_FREQ      (12500)       // 默认定义电机PWM的频率 12.5K Hz
#define Servo_FREQ      (50)          // 定义舵机和无感无刷驱动控制信号的PWM频率，修改频率需要重新标定中值 ！！！
#define Servo1          0             // 舵机编号
#define Servo2          1             // 舵机编号
#define Servo_Median    (1510)        // 定义舵机 中值PWM占空比时间@50Hz,修改频率需要重新标定中值 ！！！
/* ===============================================================================================
    0、舵机无刷PWM周期=(1000000 / Servo_FREQ) , 中值和范围自己根据频率标定                           =
    1、舵机控制信号频率50Hz，周期20ms，脉宽范围0.5ms~2.5ms，180度舵机 对应角度-90°~+90°               =
    2、无刷PWM，脉宽范围1ms~2ms，代表无刷驱动输出0到满速,若PWM频率50Hz,则对应本库duty--> 1000~2000    =
   ===============================================================================================
*/

extern s16 Motor_PWM_MAX;                    /* 根据初始化电机的频率自计算，最大占空比值的设置范围，用户无需关心 */

void Motor_Init(u16 freq);                   //电机PWM及方向IO初始化
void Servo_Init(u16 freq);
void BLmotor_Init(u16 freq);

void BLmotor_Init_1(u16 freq);
void BLmotor_Ctrl_w1(u16 BLmotor_duty);

void Motor_Ctrl(int16 Motor1,int16 Motor2);  //电机驱动控函数声明
void Servo_Ctrl(u8 Sx, u16 Sduty);
void BLmotor_Ctrl(u16 BLmotor1,u16 BLmotor2);


#endif
