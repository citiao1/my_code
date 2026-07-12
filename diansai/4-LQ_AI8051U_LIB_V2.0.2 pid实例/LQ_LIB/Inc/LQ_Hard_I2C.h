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

#ifndef __LQ_HARD_IIC_H_
#define __LQ_HARD_IIC_H_

#include "AI8051U_I2C.h"

void Hard_IIC_Wait();
void Hard_IIC_Start();
void Hard_IIC_SendData(unsigned char dat);
void Hard_IIC_RecvACK();
unsigned char Hard_IIC_RecvData();
void Hard_IIC_SendACK();
void Hard_IIC_SendNAK();
void Hard_IIC_Stop();
void Hard_IIC_Init(unsigned char type);
void Hard_I2C_ReadNbyte(u8 dev_addr, u8 mem_addr, u8 *p, u8 number);
void Hard_I2C_WriteNbyte(u8 dev_addr, u8 mem_addr, u8 *p, u8 number);

#endif
