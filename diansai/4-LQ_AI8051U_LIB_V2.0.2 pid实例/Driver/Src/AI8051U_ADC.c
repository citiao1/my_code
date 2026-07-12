/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/
#include "AI8051U_ADC.h"



/***********************************************************************************
 * 函 数 名：void ADC_GPIO_Init(ADC_CHx CHn_IOx)
 * 功    能：ADC管脚初始化,模拟输入模式,高阻浮空
 * 参    数：无
 * 返 回 值：无
 * 说    明：    //ADC 端口IO配置，详见ADC_CHx
 *          eg:ADC_GPIO_Init(ADC_CH0_P10); //配置ADC0输入端为P10口
 **********************************************************************************/
void ADC_GPIO_Init(ADC_CHx CHn_IOx)
{
    if (CHn_IOx < ADC_CH0_P10 || CHn_IOx > ADC_CH15_P07)        return; // 输入参数错误
    if (CHn_IOx <= ADC_CH7_P17)
    {
        GPIO_Init(GPIO_P1, ((u8)(1 << CHn_IOx)), GPIO_Mode_IN_FLOATING);     // 模拟输入模式,高阻浮空
    }
    else if (CHn_IOx <= ADC_CH15_P07)
    {
        GPIO_Init(GPIO_P0, ((u8)(1 << (CHn_IOx - 8))), GPIO_Mode_IN_FLOATING); // 模拟输入模式,高阻浮空
    }
}
//========================================================================
// 函数: u8	ADC_Inilize(ADC_InitTypeDef *ADCx)
// 描述: ADC初始化程序.
// 参数: ADCx: 结构参数,请参考adc.h里的定义.
// 返回: none.
// 版本: V1.0, 2012-10-22
//========================================================================
u8 ADC_Inilize(ADC_InitTypeDef *ADCx)
{
	ADCCFG = (ADCCFG & ~ADC_SPEED_2X16T) | ADCx->ADC_Speed; // 设置ADC工作时钟频率
	ADC_Justify(ADCx->ADC_AdjResult);						// AD转换结果对齐方式

	if (ADCx->ADC_SMPduty > 31)
		return FAIL; // 错误
	if (ADCx->ADC_CsSetup > 1)
		return FAIL; // 错误
	if (ADCx->ADC_CsHold > 3)
		return FAIL; // 错误

	ADCTIM = (ADCx->ADC_CsSetup << 7) | (ADCx->ADC_CsHold << 5) | ADCx->ADC_SMPduty; // 设置 ADC 内部时序，ADC采样时间建议设最大值
	return SUCCESS;
}

//========================================================================
// 函数: void	ADC_PowerControl(u8 pwr)
// 描述: ADC电源控制程序.
// 参数: pwr: 电源控制,ENABLE或DISABLE.
// 返回: none.
// 版本: V1.0, 2012-10-22
//========================================================================
void ADC_PowerControl(u8 pwr)
{
	if (pwr == ENABLE)
		ADC_POWER = 1;
	else
		ADC_POWER = 0;
}

//========================================================================
// 函数: u16 Get_ADCResult(ADC_CHx channel)
// 描述: 查询法读一次ADC转换结果.
// 参数: channel: 选择要转换的ADC通道.0-15
// 返回: ADC转换结果.
// 版本: V1.0, 2012-10-22
//========================================================================
u16 Get_ADCResult(ADC_CHx channel)
{
	u16 adc;
	u8 i;

	if (channel > ADC_CH15_P07)
		return 4096; // 错误,返回4096,调用的程序判断
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xf0) | channel; // 设置ADC转换通道
	ADC_START = 1;							  // 启动ADC转换
	NOP(10);								  // 对ADC_CONTR操作后要4T之后才能访问

	for (i = 0; i < 250; i++) // 超时返回，正常i等于10以内就可以转换完成
	{
		if (ADC_FLAG)
		{
			ADC_FLAG = 0; // 清除ADC转换结束标志
			if (RESFMT)	  // 转换结果右对齐。
			{
				adc = ((u16)ADC_RES << 8) | ADC_RESL;
			}
			else // 转换结果左对齐。
			{
				adc = (u16)ADC_RES;
				adc = (adc << 4) | ((ADC_RESL >> 4) & 0x0f);
			}
			return adc;
		}
	}
	return 4096; // 错误,返回4096,调用的程序判断
}
