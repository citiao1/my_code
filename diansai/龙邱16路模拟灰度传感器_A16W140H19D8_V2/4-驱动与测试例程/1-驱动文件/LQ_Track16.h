#ifndef _LQ_Track_H_
#define _LQ_Track_H_

#define TRACK_CHANNEL_NUM (16)                            /* 定义通道数 */
extern unsigned int LQ_Tracking_Value[TRACK_CHANNEL_NUM]; /* 传感器数据缓存数组 */

/* 通道选择引脚定义，注意初始化函数需对接你自己的MCU GPIO初始化函数 */
/* 通道选择总线 TRACK_SEL[3:0] */
#define TRACK_S3_PIN (P01_3)   /* 通道选择位bit:3 (MSB) */
#define TRACK_S2_PIN (P02_10)  /* 通道选择位bit:2       */
#define TRACK_S1_PIN (P00_12)  /* 通道选择位bit:1       */
#define TRACK_S0_PIN (P00_10)  /* 通道选择位bit:0 (LSB) */

void LQ_Tracking_init(void);
unsigned int Tracking_Value_once(unsigned char ch);
void Tracking_Value_Acquire(void);
void Test_LQ_Tracking(void);

#endif
