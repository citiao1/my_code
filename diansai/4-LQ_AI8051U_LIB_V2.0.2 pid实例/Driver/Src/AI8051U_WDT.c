
#include "AI8051U_WDT.h"

//========================================================================
// 函数: void WDT_Inilize(WDT_InitTypeDef *WDT)
// 描述: 看门狗初始化程序.
// 参数: WDT: 结构参数,请参考WDT.h里的定义.
// 返回: none.
// 版本: V1.0, 2020-09-16
//========================================================================
void WDT_Inilize(WDT_InitTypeDef *WDT)
{
	if (WDT->WDT_Enable == ENABLE)
		EN_WDT = 1;                 // 使能看门狗

	WDT_PS_Set(WDT->WDT_PS);        // 看门狗定时器时钟分频系数
	if (WDT->WDT_IDLE_Mode == WDT_IDLE_STOP)
		IDL_WDT = 0;                // IDLE模式停止计数
	else
		IDL_WDT = 1;                // IDLE模式继续计数
}

/********************* 清除看门狗函数 *************************/
void Feeding_Dog(void)
{
	CLR_WDT = 1; // 喂狗
}

