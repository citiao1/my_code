#ifndef __APP_SCHEDULER_H__
#define __APP_SCHEDULER_H__

/*******************************************************************************
 * @file    app_scheduler.h
 * @brief   应用层调度器接口\uff08App Layer\uff09
 *
 * @note    调度方式（学习自 MSPM0G3507_Diansai_Test 工程\uff09：
 *          - SysTick 配置成 1ms 中断，中断里只累加 app_tick_ms，不做任何计算；
 *          - 主循环 APP_Scheduler_Run() 根据"时间片"判断是否到达 5ms 周期，
 *            到了就调用 s_scheduler_step() 执行位置环 PID + 陀螺仪 + 速度环更新；
 *          - dt 用实际值 (tick - last_tick)，减小主循环抖动对积分/微分的影响。
 *
 *          这种方式比"中断里直接运 PID"好：
 *          1. 中断里只做最短的自加，主循环负责异常复杂的逻辑；
 *          2. dt 实测，依然有主循环抖动 PID 也不会跑飞；
 *          3. 主循环末尾 __WFI() 营入低功耗。
 *
 *          三环开关由 g_param.enable_pos / enable_yaw / enable_speed 控制，
 *          位置环 / 角速度环 / 速度环在 s_scheduler_step() 里顺序执行。
 *******************************************************************************/

#include "include.h"

/* 调度周期\uff08毫秒\uff09：位置环 5ms 一次\uff0cPID 默认参数也是按 5ms 调 */
#define APP_SCHED_TICK_MS              (5u)

/* 初始化 SysTick 1ms 中断 + 调度器状态
 * @note  应在 LQ_System_Init 之后调用\uff0c需要 80MHz 䳲频：
 *        SysTick LOAD=80000\uff0880MHz/80000=1ms\uff09
 *        关中断（原 LQ 库 1us 时基systick_delay/delay_us 不受影响\uff0c
 *        因为它们也写 SysTick，和本文件的 LOAD 无关）
 */
void APP_Scheduler_Init(void);

/* 主循环调度器
 * @note  应在 main() 的 while(1) 里高频调用：
 *          while (1) { APP_Scheduler_Run(); __WFI(); }
 *        每次调用会判断是否到达 APP_SCHED_TICK_MS 周期，
 *        到了就执行 s_scheduler_step()�1b未到直接返回。
 *        调用方可以进 __WFI()�0c等 SysTick 中断\uff081ms\uff09抢醒。
 */
void APP_Scheduler_Run(void);

/* 获取当前时间截（毫秒）\uff0c主循环绘图/VOFA+ 发送等可能使用 */
uint32_t APP_Scheduler_GetTickMs(void);

#endif
