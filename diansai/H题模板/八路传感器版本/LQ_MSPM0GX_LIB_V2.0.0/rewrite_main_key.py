#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""重写 main.c 和 mid_key.c (GB2312 编码)"""
import os

ROOT = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0"

# ============================================================
# 1. main.c 内容
# ============================================================
MAIN_C = r"""/*******************************************************************************
 * @file    main.c
 * @brief   差速小车主程序 - 三串级 PID 循迹控制
 *
 * @note    启动流程：
 *          1. 系统初始化（80MHz 主频）
 *          2. 初始化 PID 参数（默认急停）
 *          3. 初始化调试串口（UART0 @ 9600 波特率，VOFA+ 上位机）
 *          4. 初始化电机 PWM（10kHz）
 *          5. 初始化编码器
 *          6. 初始化 5 路红外循迹传感器（替代原 8 路模拟灰度）
 *          7. LSM6DSV16X IMU 初始化（SPI 接口）
 *          8. 底盘三环 PID 状态初始化
 *          9. 调度器初始化（SysTick 1ms 定时器中断）
 *         10. 蜂鸣器初始化 + 上电提示音（2 声"哔"）
 *         11. 主循环：高频轮询 + __WFI 低功耗
 *
 *          主循环采用"高频轮询 + __WFI 低功耗"模式：
 *          - SysTick 每 1ms 中断唤醒 CPU
 *          - APP_DEBUG_ServiceTx 推进 TX DMA 发送队列
 *          - APP_DEBUG_PollRx 轮询 RX FIFO 并解析 FireWater 协议
 *          - APP_Scheduler_Run 调度器每 5ms 执行一帧控制
 *          - APP_DEBUG_PollPlot 每 20ms 推送一帧绘图数据
 *          - MID_Key_Scan 按键轮询 20ms 一次
 *          - MID_OLED_Task OLED 分片刷屏（在 __WFI 之前）
 *******************************************************************************/

#include "include.h"
#include "LQ_device.h"
#include "LQ_lsm6dsv16x.h"
/* LQ_tracking.h 已移除（5 路红外替代原 8 路模拟灰度） */
#include "mid_motor.h"
#include "mid_encoder.h"
#include "mid_chassis.h"
#include "mid_pid.h"
#include "mid_line.h"            /* 5 路红外循迹 + 紧急停车检测 */
#include "mid_oled.h"            /* 0.96 寸 OLED 显示中间层 */
#include "mid_key.h"             /* K1 启停按键 + 20ms 轮询 */
#include "mid_imu.h"
#include "mid_beep.h"
#include "app_debug.h"
#include "app_scheduler.h"

/* ============================================================
 *  OLED 显示：本机常量
 * ============================================================ */

/* 屏幕显示刷新周期：每 50ms 刷一次，避免 GRAM 频繁脏 */
#define SCREEN_REFRESH_MS         (50u)

/* 上次刷屏的 1ms 时间戳 */
static uint32_t s_last_screen_ms = 0u;

/* ============================================================
 *  OLED 显示：辅助函数
 * ============================================================ */

/* 把 5 路原始电平格式化为字符串 "H/L H/L H/L H/L H/L "
 * @param levels  5 路原始电平（0/1）
 * @param buf     输出字符串（至少 16 字节）
 * @param states  可选输出 5 路 H/L 字符（5 字节）
 */
static void format_levels(const uint8_t *levels, char *states)
{
    int i;
    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        states[i] = levels[i] ? 'H' : 'L';
    }
    states[MID_IR_SENSOR_COUNT] = '\0';
}

/* ============================================================
 *  OLED 显示：刷新当前帧（5 路 H/L + 状态）
 *  布局：
 *    第 0 行：STATUS: RUN / STOP / E-STOP
 *    第 1 行：IR1 IR2 IR3 IR4 IR5
 *    第 2~6 行：5 路 H/L
 *    第 7 行（备用）：保留
 * ============================================================ */
static void s_screen_refresh(void)
{
    uint32_t now = APP_Scheduler_GetTickMs();
    if ((now - s_last_screen_ms) < SCREEN_REFRESH_MS) {
        return;
    }
    s_last_screen_ms = now;

    /* 1) 读 5 路原始电平（直接读 GPIO） */
    uint8_t levels[MID_IR_SENSOR_COUNT];
    MID_IR_ReadRaw(levels);

    /* 2) 构造 H/L 字符数组 */
    char states[6];
    format_levels(levels, states);

    /* 3) 渲染状态行 */
    const char *status_str;
    if (g_line_emergency_stopped) {
        status_str = "STATUS:E-STOP";
    } else if (MID_Key_IsRunning()) {
        status_str = "STATUS:  RUN";
    } else {
        status_str = "STATUS: STOP ";
    }
    MID_OLED_ShowString(0u, 0u, status_str, MID_OLED_FONT_8);

    /* 4) 渲染 5 路标签头 */
    MID_OLED_ShowString(1u, 0u, "IR1 IR2 IR3 IR4 IR5", MID_OLED_FONT_8);

    /* 5) 渲染 5 路 H/L（每行显示一个 H/L，留更多行距） */
    uint8_t x_pos = 6u;        /* 横向偏移，使字符居中 */
    for (int i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        char buf[2] = {states[i], '\0'};
        MID_OLED_ShowString((uint8_t)(2u + i), x_pos + (uint8_t)(i * 12u), buf, MID_OLED_FONT_12);
    }
}

/* ============================================================
 *  主函数
 * ============================================================ */
int main(void)
{
    /* 1. 系统初始化（80MHz 主频） */
    LQ_System_Init();
    delay_ms(10);

    /* 2. 初始化全局 PID 参数（默认急停） */
    MID_Param_Init();

    /* 3. 初始化调试串口 UART0 @ 9600 波特率 */
    APP_DEBUG_Init();

    /* 4. 初始化电机 PWM（10kHz） */
    MID_Motor_Init(10000);

    /* 5. 编码器初始化 */
    MID_Encoder_Init();

    /* 6. 5 路红外循迹初始化（替代原 8 路灰度 ADC） */
    MID_IR_Init();

    /* 7. LSM6DSV16X IMU 初始化（SPI 接口） */
    uint8_t imu_id = LQ_LSM6DSV16X_Init();
    if (imu_id != LSM6DSV16X_DRV_ID) {
        while (1) {
            APP_DEBUG_Printf("IMU Init FAIL, ID=0x%02X\r\n", imu_id);
            APP_DEBUG_ServiceTx();
            delay_ms(500);
        }
    }
    delay_ms(100);

    /* 8. 底盘三环 PID 状态初始化 */
    MID_Chassis_Init();

    /* 9. 调度器初始化（SysTick 1ms 定时器中断） */
    APP_Scheduler_Init();

    /* 10. 按键初始化（K1 启停） */
    MID_Key_Init();

    /* 11. OLED 初始化 */
    MID_OLED_Init();

    /* 12. 蜂鸣器初始化 + 上电提示音（2 声"哔"） */
    MID_Beep_Init();
    MID_Beep_Beep(2, 0.08f, 0.08f);

    /* 13. 主循环：高频轮询 + __WFI 低功耗 */
    while (1) {
        /* TX DMA 推进：从环形队列提交一帧数据到 DMA */
        APP_DEBUG_ServiceTx();

        /* RX 轮询：读取 RX FIFO 并解析 FireWater 协议 */
        APP_DEBUG_PollRx();

        /* 调度器：每 5ms 帧执行控制（5 路红外读取 + 紧急检测 + 速度环） */
        APP_Scheduler_Run();

        /* 绘图：每 20ms 推一帧数据 */
        APP_DEBUG_PollPlot();

        /* 5 路全 0/全 1 紧急停车后蜂鸣报警 3 声"哔哔哔" */
        if (g_line_emergency_stopped) {
            static uint8_t s_alarm_done = 0u;
            if (s_alarm_done == 0u) {
                s_alarm_done = 1u;
                MID_Beep_Beep(3, 0.06f, 0.06f);
            }
        }

        /* 按键周期扫描（20ms 一次） */
        MID_Key_Scan();

        /* OLED 屏幕刷新（50ms 一次） */
        s_screen_refresh();

        /* OLED 分片刷屏：每次 1 页 128 字节，8 页约 40ms */
        MID_OLED_Task();

        /* 再次推进 TX DMA，确保所有数据发出 */
        APP_DEBUG_ServiceTx();

        /* 等待下一个 SysTick 中断（1ms）唤醒 */
        __WFI();
    }
}
"""

# ============================================================
# 2. mid_key.c 内容
# ============================================================
KEY_C = r"""/*******************************************************************************
 * @file    mid_key.c
 * @brief   按键中间层实现（Middle Layer）
 *
 * @note    本文件是工程内唯一接触 LQ_key.h 的源文件
 *          业务层只通过 mid_key.h 提供的 API 使用按键
 *
 *          K1 启停状态机：
 *            上电默认 = STOPPED
 *            短按 K1：STOPPED → 校准 → RUNNING → STOPPED → ...
 *
 *          启停反馈：
 *            - 启动（STOPPED → RUNNING）：1 声短"滴"
 *            - 急停（RUNNING → STOPPED）：2 声短"滴滴"
 *
 *          紧急停车集成：
 *            - 5 路红外全 0/全 1 时，由 mid_line.c 触发紧急停车
 *              并置 g_line_emergency_stopped=1
 *            - K1 启动时（STOPPED → RUNNING）自动清该标志
 *              允许 5 路异常恢复后重新启动
 *
 * @author  LQ_012
 * @date    2026-07-27
 *******************************************************************************/

#include "mid_key.h"
#include "app_scheduler.h"     /* APP_Scheduler_GetTickMs（1ms 时基） */
#include "mid_chassis.h"        /* MID_Chassis_Start / Stop            */
#include "mid_pid.h"            /* g_param.chassis_vx                  */
#include "mid_imu.h"            /* MID_IMU_CalibrateGyro               */
#include "mid_beep.h"           /* MID_Beep_Beep                       */
#include "mid_line.h"           /* g_line_emergency_stopped            */
#include "LQ_key.h"             /* 唯一允许的硬件头文件，仅本文件 include */

/* ============================================================
 *  内部宏
 * ============================================================ */

/* 扫描周期：每 20ms 实调一次 LQ_Key_Scan（与 IMU 按键周期一致） */
#define KEY_SCAN_INTERVAL_MS     (20u)

/* ============================================================
 *  内部状态（本文件私有）
 * ============================================================ */

/* 上次按键扫描的 1ms 时间戳，用于降频 */
static uint32_t s_last_key_scan_ms = 0u;

/* 启停状态机：0 = 急停（默认），1 = 运行中 */
static uint8_t s_running = 0u;

/* ============================================================
 *  对外 API
 * ============================================================ */

/* 查询当前是否处于运行状态
 * 详细说明：mid_key.h
 */
uint8_t MID_Key_IsRunning(void)
{
    return s_running;
}

/* 初始化按键 GPIO
 * 详细说明：mid_key.h
 */
void MID_Key_Init(void)
{
    LQ_Key_Init();
    /* 上电默认 = 急停（与 param.c 默认 enable=0 一致） */
    s_running = 0u;
    s_last_key_scan_ms = 0u;
}

/* K1 短按：急停 → 启动分支
 * @note  先调陀螺仪校准（耗时约 1s，期间主循环阻塞），
 *        校准完成后再开三环使能
 */
static void s_handle_start(void)
{
    /* 1) 启动前自动校准陀螺仪（先关三环保护 + 停电机 + 蜂鸣提示） */
    MID_IMU_CalibrateGyro();

    /* 2) 恢复默认目标速度（param.c 默认 40 mm/s，Stop 时已被清 0） */
    g_param.chassis_vx = __chassis_vx;

    /* 3) 清除 5 路红外紧急停车标志（5 路全 0/全 1 触发的停车状态） */
    g_line_emergency_stopped = 0u;

    /* 4) 打开三环使能 */
    MID_Chassis_Start();

    /* 5) 标记为运行中 */
    s_running = 1u;

    /* 6) 启动成功提示音：1 声短"滴" */
    MID_Beep_Beep(1, 0.08f, 0.0f);
}

/* K1 短按：启动 → 急停分支 */
static void s_handle_stop(void)
{
    /* 关三环 + 清速度 + 强制停电机 */
    MID_Chassis_Stop();
    s_running = 0u;

    /* 急停提示音：2 声短"滴滴" */
    MID_Beep_Beep(2, 0.08f, 0.08f);
}

/* K1 短按事件处理（内部函数） */
static void s_handle_k1_press(void)
{
    if (s_running == 0u) {
        s_handle_start();
    } else {
        s_handle_stop();
    }
}

/* 按键周期扫描
 * 详细说明：mid_key.h
 */
void MID_Key_Scan(void)
{
    /* 1) 降频：每 20ms 才实调一次 LQ_Key_Scan */
    uint32_t now = APP_Scheduler_GetTickMs();
    if ((now - s_last_key_scan_ms) < KEY_SCAN_INTERVAL_MS) {
        return;
    }
    s_last_key_scan_ms = now;

    /* 2) K1 短按（按下瞬间 = LQ_Key_Scan 返回 1）：切换 启停 状态 */
    if (LQ_Key_Scan(KEY1) == 1) {
        s_handle_k1_press();
    }

    /* 3) K0 / K2 短按：预留，当前不绑定动作
     *    如后续需要，参考 s_handle_k1_press() 添加 handler
     */
    (void)LQ_Key_Scan(KEY0);
    (void)LQ_Key_Scan(KEY2);
}
"""

# ============================================================
# 写入文件（GB2312 编码）
# ============================================================

for path, content in [
    (os.path.join(ROOT, "User", "main.c"), MAIN_C),
    (os.path.join(ROOT, "Code", "Middle", "mid_key.c"), KEY_C),
]:
    with open(path, "wb") as f:
        f.write(content.encode("gb2312"))
    print(f"WROTE: {path} ({len(content.encode('gb2312'))} bytes)")
