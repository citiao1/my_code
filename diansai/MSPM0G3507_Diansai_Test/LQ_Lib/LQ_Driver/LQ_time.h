/*******************************************************************************
 * @file                LQ_time.h
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
#ifndef __LQ_TIME_H__
#define __LQ_TIME_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"

/*!
 * @brief       定时器通道枚举
 * 
 * @note        默认总线时钟为80M
 * @note        注意 LQ_TIMERG_0, LQ_TIMERG_6 总线时钟只有40Mhz
 * 
 * @note        注意 LQ_TIMERG_4 是 32bit 定时器 Prescaler 只能设置成 0
 *              计数范围是 0 - 4,294,967,295 足够覆盖所有计数范围
 */
typedef enum {
	
    /* TIMA0 */         /* TIMA1 */                            
    LQ_TIMERA_0,        LQ_TIMERA_1,

    /* TIMG0 */         /* TIMG6 */         /* TIMG7 */         /* TIMG8 */
    LQ_TIMERG_0,        LQ_TIMERG_6,        LQ_TIMERG_7,        LQ_TIMERG_8,

    /* TIMG12 */
    LQ_TIMERG_12,

    LQ_TIMER_ALL,   /* 定时器总数 */
	
} LQEnum_Timer_t;

/*!
 * @brief       定时器回调函数数组
 */
extern LQ_INT_CallBack_t LQ_TIM_IT_CallBack[LQ_TIMER_ALL];

/*!
 * @brief       定时器配置结构体
 * 
 * @note        定时器频率 = 输入时钟 / DivideRatio / Prescaler
 *              重载时间 = 定时器频率 * Period
 */
typedef struct {
	
    DL_TIMER_CLOCK_DIVIDE	DivideRatio;		/* 分割器 对输入时钟先分割*/
    
    uint8_t 				Prescaler;			/* 分频器 0 - 255 对应1 - 256分频 */
    
    uint32_t 				Period;				/* 装载值 TIMERG12 为32位 其余为16位 */

    DL_TIMER_TIMER_MODE 	TimerMode;			/* 定时器计数模式 */
    
    bool 					startTimer;			/* 立即开始 */
	
} LQConfig_Timer_InitTypeDef_t;

/**
 * @brief       定时器寄存器组
 */
extern GPTIMER_Regs * const LQ_TIMER_Regs[7];

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

/*! @brief	定时器初始化 */
void LQ_TIMER_BaseInit(LQEnum_Timer_t TIMER, LQConfig_Timer_InitTypeDef_t *Timer_Init);

/*! @brief	定时器中断配置 */
void LQ_TIMER_ITConfig(LQEnum_Timer_t TIMER, uint32_t TIMER_IT, LQEnum_NVIC_Priority_t Priority);

/*! @brief	开启定时器中断 */
void LQ_TIMER_ITEnable(LQEnum_Timer_t TIMER);

/*! @brief	设置定时器重装载中断回调函数 */
void LQ_TIMER_SetLoadCallback(LQEnum_Timer_t TIMER, LQ_INT_CallBack_t cb);

/*! @brief	启动定时器 */
void LQ_TIMER_Start(LQEnum_Timer_t TIMER);

/*! @brief	停止定时器 */
void LQ_TIMER_Stop(LQEnum_Timer_t TIMER);

/*! @brief	获取定时器当前计数值 */
uint32_t LQ_TIMER_GetCount(LQEnum_Timer_t TIMER);

/*! @brief	设置定时器 LOAD 值(重装载值) */
void LQ_TIMER_SetLoadValue(LQEnum_Timer_t TIMER, uint32_t Value);

/*! @brief	设置定时器计数值 */
void LQ_TIMER_SetCountValue(LQEnum_Timer_t TIMER, uint32_t Value);

#endif
