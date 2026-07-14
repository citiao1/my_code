#ifndef _LQ_TRACKING_H
#define _LQ_TRACKING_H

#include "include.h"

//-----------------8 路模拟量循迹模块三个 IO 引脚控制宏定义-----------------

#define Tracking_S0_LOW			DL_GPIO_clearPins(Tracking_PORT, Tracking_S0_PIN)  // 输出低电平
#define Tracking_S1_LOW			DL_GPIO_clearPins(Tracking_PORT, Tracking_S1_PIN)  // 输出低电平
#define Tracking_S2_LOW			DL_GPIO_clearPins(Tracking_PORT, Tracking_S2_PIN)  // 输出低电平

#define Tracking_S0_HIGH		DL_GPIO_setPins(Tracking_PORT, Tracking_S0_PIN)  // 输出高电平
#define Tracking_S1_HIGH		DL_GPIO_setPins(Tracking_PORT, Tracking_S1_PIN)  // 输出高电平
#define Tracking_S2_HIGH		DL_GPIO_setPins(Tracking_PORT, Tracking_S2_PIN)  // 输出高电平


extern unsigned char LQ_Tracking_Value[8];


void Tracking_Adc_Init();


void Tracking_IO_Set(unsigned char S2, unsigned char S1, unsigned char S0);
extern volatile bool Tracking_Adc_Check;
unsigned int Tracking_Adc_once(void);
unsigned int Tracking_Value_once(unsigned char ch);


void Tracking_Value_Acquire();

void LQ_Test_Tracking();


#endif


