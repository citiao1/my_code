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

#ifndef __LQ_KEY_H_
#define __LQ_KEY_H_

#include "config.h"

//-------------------------宏定义KEY管脚_ 切记与KEY_Init()初始化保持一致-------------------------

#define KEY0_PIN       P2_7              //母板上按键0
#define KEY1_PIN       P4_5              //母板上按键1
#define KEY2_PIN       P4_6              //母板上按键2

#define DSW0_PIN       P5_6              //母板上拨码开关SW1
#define DSW1_PIN       P5_7              //母板上拨码开关SW2

#define Tubep_PIN      P3_3              //母板上干簧管,建议使用外部中断读取，此处仅作演示


//定义模块号
typedef enum
{
    KEY0  = 0,  	            //母板上按键0
    KEY1  ,		        //母板上按键1
    KEY2  ,		        //母板上按键2
    DSW0  ,               //母板上拨码开关1
    DSW1  ,		        //母板上拨码开关2
    Tubep ,            //干簧管 接口
} KEYn_e;

//定义状态
typedef enum
{
    KEY_LOW    = 0x00,  		 //按下
    KEY_HIGH   = 0x01, 		     //松开
    KEY_FAILED = 0xFF, 	        //错误
} KEYs_e;



void GPIO_KEY_Init (void);
u8 KEY_Read (KEYn_e KEYno);


#endif



