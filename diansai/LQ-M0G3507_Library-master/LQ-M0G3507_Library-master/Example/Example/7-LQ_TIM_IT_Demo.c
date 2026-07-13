/*******************************************************************************
 * @file                7-LQ_TIM_IT_Demo.c
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
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include "LQ_Demo.h"

/*************************************************************************
 * @name     LQ_TIM_IT_Handler
 *
 * @brief    定时器中断回调函数
 *
 * @param    none
 * @return   none
 * 
 * @note     定时器中断回调函数，当定时器到达装载值时，中断回调函数被调用。
 *************************************************************************/
void LQ_TIM_IT_Handler(void)
{
    printf("Timer IT\r\n");
}

/*************************************************************************
 * @name     LQ_TIM_IT_Demo
 *
 * @brief    定时器中断例程
 *
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了定时器中断的使用方法，通过设置定时器的装载值，
 *           并使定时器在到达装载值时产生中断，并设置中断回调函数，当定时器到达装载值时，中断回调函数被调用。
 * 
 * @note     注意例程中使用的 TIMERG0 定时器原始频率只有 40MHz
 * @note     注意区别不同定时器的频率, 在 LQ_time.h 中可以查询
 *************************************************************************/
void LQ_TIM_IT_Demo(void)
{
    LQConfig_Timer_InitTypeDef_t timer_init = {
        .DivideRatio = DL_TIMER_CLOCK_DIVIDE_8,             // 输入时钟分割器 8 分频 80Mhz / 8 = 10Mhz
        .Prescaler   = 100 - 1,                             // 分频器 100 分频 10Mhz / 100 = 100000Hz = 10us
        .Period      = 50000 - 1,                           // 重载值 50000 个 10us =  10us * 50000us = 500ms
        .TimerMode   = DL_TIMER_TIMER_MODE_PERIODIC_UP,     // 设置 周期向上计数模式
        .startTimer  = false,                               // 立即开始 否 
    };

    LQ_TIMER_BaseInit(LQ_TIMERA_1, &timer_init);                // 初始化定时器
    LQ_TIMER_ITConfig(LQ_TIMERA_1,                              // 使能更新中断
        DL_TIMER_INTERRUPT_LOAD_EVENT,                      // 使能装载事件中断
        NVIC_Priority_NONE);                                // 设置中断优先级
    LQ_TIMER_ITEnable(LQ_TIMERA_1);                             // 使能定时器中断
    LQ_TIMER_Start(LQ_TIMERA_1);                                // 启动定时器

    LQ_TIMER_SetLoadCallback(LQ_TIMERA_1, LQ_TIM_IT_Handler);   // 设置装载事件中断回调函数

    while(1)
    {
        delay_ms(10);
    }
}
