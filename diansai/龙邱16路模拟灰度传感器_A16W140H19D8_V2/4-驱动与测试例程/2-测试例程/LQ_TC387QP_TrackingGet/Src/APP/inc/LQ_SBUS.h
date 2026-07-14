
#ifndef _LQ_SBUS_H_
#define _LQ_SBUS_H_

#include "lq_include.h"

extern int num3;
extern unsigned char ReadBuff3[512];
extern char Usart3_Rec_Fini_Flag; // 串口3接收完成标志位

extern unsigned int RIGHTH, LEFTV, RIGHTV, LEFTH, VRH, VRF, SWC, SWD, SWA, SWB;
extern unsigned int CHDATA[18];

void update_sbus(void);
void Test_SBUS(void);
void R9DS_Read(void);

#endif // _LQ_SBUS_H_
