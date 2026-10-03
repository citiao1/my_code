/*******************************************************************************
 * @file                LQ_isr.c
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author				wuwu	(接口层编写)
 * @author              LQ_012	(优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_exti.h"
#include "LQ_common.h"

#include "include.h"
#include "LQ_device.h"


/* UART0 中断服务函数，内部调用，用户无需关心 */
void UART0_IRQHandler(void)
{
	// 触发串口接收中断
	if (DL_UART_getPendingInterrupt(UART0) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART0] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART0]();
		}
	}
}

/* UART1 中断服务函数，内部调用，用户无需关心 */
void UART1_IRQHandler(void)
{
	// 触发串口接收中断
	if (DL_UART_getPendingInterrupt(UART1) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART1] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART1]();
		}
	}
}

/* UART2 中断服务函数，内部调用，用户无需关心 */
void UART2_IRQHandler(void)
{
	// 触发串口接收中断
	if (DL_UART_getPendingInterrupt(UART2) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART2] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART2]();
		}
	}
}

/* UART3 中断服务函数，内部调用，用户无需关心 */
void UART3_IRQHandler(void)
{
	// 触发串口接收中断
	if (DL_UART_getPendingInterrupt(UART3) == DL_UART_IIDX_RX)
	{
		if (LQ_UART_IT_CallBack[LQ_UART3] != NULL)
		{
			LQ_UART_IT_CallBack[LQ_UART3]();
		}
	}
}

/* TIMA0 中断服务函数，内部调用，用户无需关心 */
void TIMA0_IRQHandler(void)
{
	// 该定时器已被编码器占用，故无特殊情况尽量不要使用该定时器
	if (DL_TimerA_getPendingInterrupt(TIMA0) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERA_0] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERA_0]();
		}
	}
}

/* TIMA1 中断服务函数，内部调用，用户无需关心 */
void TIMA1_IRQHandler(void)
{
    if (DL_TimerA_getPendingInterrupt(TIMA1) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERA_1] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERA_1]();
		}
	}
}

/* TIMG0 中断服务函数，内部调用，用户无需关心 */
void TIMG0_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG0) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_0] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_0]();
		}
	}
}

/* TIMG6 中断服务函数，内部调用，用户无需关心 */
void TIMG6_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG6) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_6] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_6]();
		}
	}
}

/* TIMG7 中断服务函数，内部调用，用户无需关心 */
void TIMG7_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG7) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_7] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_7]();
		}
	}
}

/* TIMG8 中断服务函数，内部调用，用户无需关心 */
void TIMG8_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG8) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_8] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_8]();
		}
	}
}

/* TIMG12 中断服务函数，内部调用，用户无需关心 */
void TIMG12_IRQHandler(void)
{
	if (DL_TimerG_getPendingInterrupt(TIMG12) == DL_TIMER_IIDX_LOAD)
	{
		if (LQ_TIM_IT_CallBack[LQ_TIMERG_12] != NULL)
		{
			LQ_TIM_IT_CallBack[LQ_TIMERG_12]();
		}
	}
}

/* 外部中断 中断服务函数，内部调用，用户无需关心  */
void GROUP1_IRQHandler(void)
{
	LQ_Encoder_IRQHandlers();
    // switch ( DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) )
    // {
    // case DL_INTERRUPT_GROUP1_IIDX_GPIOA:    /* GPIOA 中断 */
    //     /* code */

    //     break;
    
    // case DL_INTERRUPT_GROUP1_IIDX_GPIOB:    /* GPIOB 中断 */
    //     /* code */
        
    //     break;
    // }
}

/* ADC0 中断服务函数，内部调用，用户无需关心 */
void ADC0_IRQHandler(void)
{
	//查询并清除ADC中断
	switch (DL_ADC12_getPendingInterrupt(ADC0))
	{	
		case DL_ADC12_IIDX_MEM0_RESULT_LOADED:		/* 通道0转换完成 */

			break;

		case DL_ADC12_IIDX_MEM1_RESULT_LOADED:		/* 通道1转换完成 */

			break;

		case DL_ADC12_IIDX_MEM2_RESULT_LOADED:		/* 通道2转换完成 */
		
			break;

		case DL_ADC12_IIDX_MEM3_RESULT_LOADED:		/* 通道3转换完成 */
		
			break;

		case DL_ADC12_IIDX_MEM4_RESULT_LOADED:		/* 通道4转换完成 */
		
			break;

		case DL_ADC12_IIDX_MEM5_RESULT_LOADED:		/* 通道5转换完成 */
		
			break;

		case DL_ADC12_IIDX_MEM6_RESULT_LOADED:		/* 通道6转换完成 */
		
			break;

		case DL_ADC12_IIDX_MEM7_RESULT_LOADED:		/* 通道7转换完成 */
		
			break;

		case DL_ADC12_IIDX_MEM8_RESULT_LOADED:		/* 通道8转换完成 */
			
			break;
		
		default:
			break;
	}
}

/* ADC1 中断服务函数，内部调用，用户无需关心 */
void ADC1_IRQHandler(void)
{
	//查询并清除ADC中断
	switch (DL_ADC12_getPendingInterrupt(ADC1))
	{	
		case DL_ADC12_IIDX_MEM0_RESULT_LOADED:		/* 通道0转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM1_RESULT_LOADED:		/* 通道1转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM2_RESULT_LOADED:		/* 通道2转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM3_RESULT_LOADED:		/* 通道3转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM4_RESULT_LOADED:		/* 通道4转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM5_RESULT_LOADED:		/* 通道5转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM6_RESULT_LOADED:		/* 通道6转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM7_RESULT_LOADED:		/* 通道7转换完成 */
			
			break;

		case DL_ADC12_IIDX_MEM8_RESULT_LOADED:		/* 通道8转换完成 */
			
			break;
		
		default:
			break;
	}
}

/* DMA 中断服务函数，内部调用，用户无需关心 */
void DMA_IRQHandler(void)
{
    
}
