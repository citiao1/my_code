/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#include "AI8051U_Exti.h"

//========================================================================
// 函数: u8	Ext_Inilize(u8 EXT, EXTI_InitTypeDef *INTx)
// 描述: 外部中断初始化程序.
// 参数: EXT: 外部中断号, INTx: 结构参数,请参考Exti.h里的定义.
// 返回: 成功返回 SUCCESS, 错误返回 FAIL.
// 版本: V1.0, 2012-10-22
//========================================================================
u8 Ext_Inilize(u8 EXT, EXTI_InitTypeDef *INTx)
{
	if (EXT > EXT_INT4_P30)		return FAIL; // 空操作

	if (EXT == EXT_INT0_P32) // 外中断0
	{
		IE0 = 0; // 外中断0标志位
		INT0_Mode(INTx->EXTI_Mode);
		return SUCCESS; // 成功
	}

	if (EXT == EXT_INT1_P33) // 外中断1
	{
		IE1 = 0; // 外中断1标志位
		INT1_Mode(INTx->EXTI_Mode);
		return SUCCESS; // 成功
	}
	//---------------------------- 以下中断没有模式可配置 默认即下降沿触发 ------------------------------
	if (EXT == EXT_INT2_P36) // 外中断2
	{
		INT2IF = 0;		// 外中断2标志位
		EX2 = 1;		// 使能 INT2 下降沿中断
		return SUCCESS; // 成功
	}

	if (EXT == EXT_INT3_P37) // 外中断3
	{
		INT3IF = 0;		// 外中断3标志位
		EX3 = 1;		// 使能 INT3 下降沿中断
		return SUCCESS; // 成功
	}

	if (EXT == EXT_INT4_P30) // 外中断3
	{
		INT4IF = 0;		// 外中断4标志位
		EX4 = 1;		// 使能 INT4 下降沿中断
		return SUCCESS; // 成功
	}
	return FAIL; // 失败
}
