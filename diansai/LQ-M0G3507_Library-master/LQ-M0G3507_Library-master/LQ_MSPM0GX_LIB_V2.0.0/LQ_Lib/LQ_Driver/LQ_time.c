/*******************************************************************************
 * @file                LQ_time.c
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
#include "LQ_time.h"
#include "LQ_gpio.h"

/*!
 * @brief       定时器回调函数数组
 */
LQ_INT_CallBack_t LQ_TIM_IT_CallBack[LQ_TIMER_ALL] = {NULL};

/*!
 * @brief       定时器寄存器映射表
 */
GPTIMER_Regs * const LQ_TIMER_Regs[] = {TIMA0, TIMA1, TIMG0, TIMG6, TIMG7, TIMG8, TIMG12};

/*!
 * @brief       定时器中断 IRQn 映射表
 */
static IRQn_Type LQ_TIMER_IRQns[] = {
    TIMA0_INT_IRQn,
    TIMA1_INT_IRQn,
    TIMG0_INT_IRQn,
    TIMG6_INT_IRQn,
    TIMG7_INT_IRQn,
    TIMG8_INT_IRQn,
    TIMG12_INT_IRQn,
};

/*************************************************************************
 * @brief       定时器初始化
 * 
 * @param       TIMER
 * @param       Timer_Init      定时器初始化结构体
 * 
 * @example     LQConfig_Timer_InitTypeDef time_config = {
 *                  .DivideRatio = DL_TIMER_CLOCK_DIVIDE_8,
 *                  .Prescaler   = 100 - 1,
 *                  .Period      = 49999,
 *                  .TimerMode   = DL_TIMER_TIMER_MODE_PERIODIC_UP,
 *                  .startTimer  = false
 *              };
 *              LQ_TIMER_BaseInit(LQ_TIMERA_0, &time_config);
 *************************************************************************/
void LQ_TIMER_BaseInit(LQEnum_Timer_t TIMER, LQConfig_Timer_InitTypeDef_t *Timer_Init)
{
    GPTIMER_Regs *timer = LQ_TIMER_Regs[TIMER];

    timer->CLKSEL = (uint32_t)DL_TIMER_CLOCK_BUSCLK;

    timer->CLKDIV = (uint32_t)Timer_Init->DivideRatio;

    timer->COMMONREGS.CPS = (uint32_t)Timer_Init->Prescaler;

    DL_Timer_setLoadValue(timer, Timer_Init->Period);

    switch (Timer_Init->TimerMode) {
        /* All four cases below should set counter value to ZERO when enabled */
        case DL_TIMER_TIMER_MODE_ONE_SHOT_UP:
        case DL_TIMER_TIMER_MODE_PERIODIC_UP:
        case DL_TIMER_TIMER_MODE_ONE_SHOT_UP_DOWN:
        case DL_TIMER_TIMER_MODE_PERIODIC_UP_DOWN:
            DL_Timer_setCounterValueAfterEnable(
                timer, DL_TIMER_COUNT_AFTER_EN_ZERO);
            break;
        /* The two cases below should set counter value to LOAD when enabled */
        case DL_TIMER_TIMER_MODE_ONE_SHOT:
        case DL_TIMER_TIMER_MODE_PERIODIC:
            DL_Timer_setCounterValueAfterEnable(
                timer, DL_TIMER_COUNT_AFTER_EN_LOAD_VAL);
            break;
        default:
            /* Code should not reach this case */
            break;
    }

    DL_Timer_setCaptureCompareValue(
        timer, 0, DL_TIMER_CC_0_INDEX);

    DL_Timer_setCaptureCompareCtl(timer, (uint32_t)DL_TIMER_INTERM_INT_DISABLED,
        DL_TIMER_CC_ACOND_TIMCLK, DL_TIMER_CC_0_INDEX);

    /* CCCTL0 should be used by default for timer. */
    DL_Timer_setCounterControl(timer, DL_TIMER_CZC_CCCTL0_ZCOND,
        DL_TIMER_CAC_CCCTL0_ACOND, DL_TIMER_CLC_CCCTL0_LCOND);

    DL_Common_updateReg(&timer->COUNTERREGS.CTRCTL,
        ((uint32_t) Timer_Init->TimerMode | (uint32_t) Timer_Init->startTimer),
        (GPTIMER_CTRCTL_REPEAT_MASK | GPTIMER_CTRCTL_EN_MASK |
            GPTIMER_CTRCTL_CM_MASK));

    DL_TimerA_enableClock(timer);
}

/*************************************************************************
 * @brief       定时器归零中断回调函数设置
 * 
 * @param       TIMER           定时器枚举
 * @param       cb              中断回调函数
 * 
 * @example     LQ_TIMER_SetLoadCallback(LQ_TIMERA_0, my_timer_zero_callback);
 *************************************************************************/
void LQ_TIMER_SetLoadCallback(LQEnum_Timer_t TIMER, LQ_INT_CallBack_t cb)
{
    LQ_TIM_IT_CallBack[TIMER] = cb;
}

/*************************************************************************
 * @brief       定时器中断设置
 * 
 * @param       TIMER           定时器枚举
 * @param       TIMER_IT        中断标志位, 从下面列出的中断中选择
 *              @arg    DL_TIMER_INTERRUPT_ZERO_EVENT           过零事件
 *              @arg    DL_TIMER_INTERRUPT_LOAD_EVENT           重装载事件
 *              @arg    ...
 * @param       Priority        中断优先级
 * 
 * @example     LQ_TIMER_ITConfig(LQ_TIMERA_0, DL_TIMER_INTERRUPT_OVERFLOW_EVENT, NVIC_Priority_NONE);
 *************************************************************************/
void LQ_TIMER_ITConfig(LQEnum_Timer_t TIMER, uint32_t TIMER_IT, LQEnum_NVIC_Priority_t Priority)
{
    DL_Timer_enableInterrupt(LQ_TIMER_Regs[TIMER], TIMER_IT);

    NVIC_ClearPendingIRQ(LQ_TIMER_IRQns[TIMER]);

    if(Priority == NVIC_Priority_NONE) return;

    NVIC_SetPriority(LQ_TIMER_IRQns[TIMER], Priority);
}

/*************************************************************************
 * @brief       开启定时器中断
 * 
 * @param       TIMER           定时器枚举
 * 
 * @example     Lq_TIMER_ITEnable(LQ_TIMERA_0);
 *************************************************************************/
void LQ_TIMER_ITEnable(LQEnum_Timer_t TIMER)
{
    NVIC_EnableIRQ(LQ_TIMER_IRQns[TIMER]);
}

/*************************************************************************
 * @brief       启动定时器
 * 
 * @param       TIMER           定时器枚举
 * 
 * @example     Lq_TIMER_Start(LQ_TIMERA_0);
 *************************************************************************/
void LQ_TIMER_Start(LQEnum_Timer_t TIMER)
{
    LQ_TIMER_Regs[TIMER]->COUNTERREGS.CTRCTL |= (GPTIMER_CTRCTL_EN_ENABLED);
}

/*************************************************************************
 * @brief       停止定时器
 * 
 * @param       TIMER           定时器枚举
 * 
 * @example     Lq_TIMER_Start(LQ_TIMERA_0);
 *************************************************************************/
void LQ_TIMER_Stop(LQEnum_Timer_t TIMER)
{
    LQ_TIMER_Regs[TIMER]->COUNTERREGS.CTRCTL &= ~(GPTIMER_CTRCTL_EN_ENABLED);
}

/*************************************************************************
 * @brief       获取定时器当前计数值
 * 
 * @param       TIMER           定时器枚举
 * 
 * @return      当前计数值
 * 
 * @example     uint32_t count = Lq_TIMER_GetCount(LQ_TIMERA_0);
 *************************************************************************/
uint32_t LQ_TIMER_GetCount(LQEnum_Timer_t TIMER)
{
    return (LQ_TIMER_Regs[TIMER]->COUNTERREGS.CTR & GPTIMER_CTR_CCTR_MASK);
}

/*************************************************************************
 * @brief       设置定时器 LOAD 值（重装载值）
 * 
 * @param       TIMER           定时器枚举
 * @param       Value           LOAD值
 * 
 * @example     Lq_TIMER_SetLoadValue(LQ_TIMERA_0, 49999);
 *************************************************************************/
void LQ_TIMER_SetLoadValue(LQEnum_Timer_t TIMER, uint32_t Value)
{
    LQ_TIMER_Regs[TIMER]->COUNTERREGS.LOAD = Value;
}

/*************************************************************************
 * @brief       设置定时器计数值
 * 
 * @param       TIMER           定时器枚举
 * @param       Value           Count值
 * 
 * @example     Lq_TIMER_SetCountValue(LQ_TIMERA_0, 0);
 *************************************************************************/
void LQ_TIMER_SetCountValue(LQEnum_Timer_t TIMER, uint32_t Value)
{
    LQ_TIMER_Regs[TIMER]->COUNTERREGS.CTR = Value;
}
