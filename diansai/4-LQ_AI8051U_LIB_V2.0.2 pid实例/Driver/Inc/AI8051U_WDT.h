/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#ifndef	__STC32G_WDT_H
#define	__STC32G_WDT_H

#include "config.h"

#define D_WDT_FLAG			(1<<7)
#define D_EN_WDT			(1<<5)
#define D_CLR_WDT			(1<<4)	/* auto clear	*/
#define D_IDLE_WDT			(1<<3)	/* WDT counter when Idle	*/

#define WDT_IDLE_STOP		0
#define WDT_IDLE_RUN		1
/**************************************************************************************
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

typedef enum
{
    WDT_SCALE_2	  = 0,
    WDT_SCALE_4	  = 1,
    WDT_SCALE_8	  = 2,
    WDT_SCALE_16  = 3,
    WDT_SCALE_64  = 5,
    WDT_SCALE_32  = 4,
    WDT_SCALE_128 = 6,
    WDT_SCALE_256 = 7,
}WDT_PS_SCALE;

#define	WDT_PS_Set(n)	WDT_CONTR = (WDT_CONTR & ~0x07) | (n & 0x07)		    /* 看门狗定时器时钟分频系数设置 */
#define	WDT_reset(n)	WDT_CONTR = D_EN_WDT + D_CLR_WDT + D_IDLE_WDT + (n)		/* 初始化WDT，喂狗 */



typedef struct
{
	u8	WDT_Enable;			//看门狗使能  ENABLE,DISABLE
	u8	WDT_IDLE_Mode;		//IDLE模式停止计数	WDT_IDLE_STOP,WDT_IDLE_RUN
	u8	WDT_PS;				//看门狗定时器时钟分频系数WDT_SCALE_2 ,,,,WDT_SCALE_256
} WDT_InitTypeDef;

void WDT_Inilize(WDT_InitTypeDef *WDT);
void Feeding_Dog(void);

#endif
