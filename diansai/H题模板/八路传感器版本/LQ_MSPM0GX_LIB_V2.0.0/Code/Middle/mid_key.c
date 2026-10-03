/*******************************************************************************
 * @file    mid_key.c
 * @brief   按键中间层模块实现
 *
 * @note    本文件只通过 LQ_key.h 提供对外接口
 *          业务只能包含 mid_key.h 使用 API 接口
 *
 *          K1 按键切换运行/停止状态
 *            初始默认状态 = STOPPED
 *            按下 K1：STOPPED -> 校准 -> RUNNING -> STOPPED -> ...
 *
 *          运行/停止逻辑说明
 *            - 从 STOPPED 切换到 RUNNING 时：蜂鸣器响1声表示"启动"
 *            - 从 RUNNING 切换到 STOPPED 时：蜂鸣器响2声表示"停止"
 *
 *          紧急停止触发条件：
 *            - 8 路灰度传感器全 0/全 1 时，由 mid_line.c 设置紧急停止标志
 *              即 g_line_emergency_stopped = 1
 *            - K1 按键按下时，若当前为 STOPPED 则切换为 RUNNING，同时清除
 *              紧急停止标志（前提是 8 路传感器正常）
 *
 * @author  LQ_012
 * @date    2026-07-27
 *******************************************************************************/

#include "mid_key.h"
#include "app_scheduler.h"     /* APP_Scheduler_GetTickMs：1ms 时间基准 */
#include "mid_chassis.h"       /* MID_Chassis_Start / Stop            */
#include "mid_pid.h"           /* g_param.chassis_vx                  */
#include "mid_imu.h"           /* MID_IMU_CalibrateGyro               */
#include "mid_beep.h"          /* MID_Beep_Beep                       */
#include "mid_line.h"          /* g_line_emergency_stopped            */
#include "mid_encoder.h"       /* MID_Encoder_OdomReset 里程归零      */
#include "LQ_key.h"            /* 唯一允许包含的底层硬件头文件        */

/* ============================================================
 *  静态变量定义
 * ============================================================ */

/* 按键扫描时间记录：用于 1ms 时间基准判断 */
static uint32_t s_last_key_scan_ms = 0u;

/* 运行状态标志：0 = 停止（默认），1 = 运行中
 * @note  保留用于兼容；MID_Key_IsRunning 实际改用 s_launch_mode 判断 */
static uint8_t s_running = 0u;

/* 当前发车模式（IDLE/K1/K2）：决定速度步进与紧急停车使能策略
 * - IDLE：停止状态，不步进
 * - K1  ：快发+按里程减速，启用脱线紧急停车
 * - K2  ：慢发+渐进加速，禁用脱线紧急停车
 * 由 K1/K2 按键置位，s_handle_stop 复位为 IDLE */
static MID_LaunchMode_t s_launch_mode = MID_MODE_IDLE;

/* 当前模式的巡航速度（ramp 收敛目标）
 * K1/K2 发车时由默认集的 chassis_vx 保存，供 MID_Chassis_RampStep 读取 */
static float s_cruise_vx = 0.0f;

/* ============================================================
 *  对外 API 实现
 * ============================================================ */

/* 查询当前系统是否处于运行状态
 * 详细说明请参考 mid_key.h
 */
uint8_t MID_Key_IsRunning(void)
{
    return (s_launch_mode != MID_MODE_IDLE) ? 1u : 0u;
}

/* 查询当前发车模式
 * 详细说明请参考 mid_key.h
 */
MID_LaunchMode_t MID_Key_GetLaunchMode(void)
{
    return s_launch_mode;
}

/* 查询当前模式的巡航速度（ramp 收敛目标）
 * @return K1/K2 发车时保存的模式巡航速度；IDLE 时返回 0
 * @note  供 MID_Chassis_RampStep 作为减速终点/加速目标的基准
 */
float MID_Key_GetCruiseVx(void)
{
    return s_cruise_vx;
}

/* 初始化按键硬件 GPIO
 * 详细说明请参考 mid_key.h
 */
void MID_Key_Init(void)
{
    LQ_Key_Init();
    /* 初始默认状态 = 停止（与 param.c 默认 enable=0 一致） */
    s_running = 0u;
    s_last_key_scan_ms = 0u;
}

/* K1 按下启动处理：停止 -> 运行 分支
 * @note  启动前会执行 IMU 陀螺仪校准，耗时约 1s，期间不响应按键循环
 *        校准完成后系统立即进入运行状态
 */
static void s_handle_start(void)
{
    /* 1) 执行 IMU 陀螺仪校准（校准期间会阻塞，并且蜂鸣器会提示） */
    MID_IMU_CalibrateGyro();
    /* 加载 K1 完整默认参数集（覆盖当前 g_param，含 PID/速度/使能） */
    MID_Param_LoadK1Defaults();
    /* 保存 K1 巡航速度（ramp 收敛目标），再加发车偏移 */
    s_cruise_vx = g_param.chassis_vx;
    /* 2) K1 快发模式：发车速度 = 巡航速度 + g_param.launch_offset（默认 +200） */
    g_param.chassis_vx = s_cruise_vx + g_param.launch_offset;

    /* 3) 清除 8 路灰度传感器的紧急停止标志（若传感器状态正常） */
    g_line_emergency_stopped = 0u;

    /* 清紧急停止滑动窗口，避免重启后因窗口残留立即再次触发 */
    MID_Line_ResetEmergency();

    /* 4) K1 模式启用紧急停车检测（脱线停车） */
    g_line_emergency_enable = 1U;

    /* 5) 里程归零：记录当前编码器累计脉冲为基准，后续里程从 0 开始累计 */
    MID_Encoder_OdomReset();

    /* 6) 启动底盘电机使能 */
    MID_Chassis_Start();
		
    /* 7) 更新运行状态标志 + 进入 K1 发车模式 */
    s_running = 1u;
    s_launch_mode = MID_MODE_K1;

    /* 8) 蜂鸣器提示：响 1 声表示"启动" */
    MID_Beep_Beep(1, 0.08f, 0.0f);
}

/* K1 按下停止处理：运行 -> 停止 分支 */
static void s_handle_stop(void)
{
    /* 停止底盘 + 清零速度 + 强制紧急停止 */
    MID_Chassis_Stop();
    s_running = 0u;
		
    /* 退出发车模式，回到 IDLE */
    s_launch_mode = MID_MODE_IDLE;

    s_cruise_vx = 0.0f;            /* 清零巡航速度 */
    /* 停止时恢复紧急停车使能，兼容现有行为（下次发车前默认使能） */
    g_line_emergency_enable = 1U;

    /* 停止提示：响 2 声表示"停止" */
    MID_Beep_Beep(2, 0.08f, 0.08f);
}

/* K2 按下启动处理：停止 -> K2 慢发渐进加速 分支
 * @note  与 K1 区别：发车速度更低（巡航速度-100）、禁用脱线紧急停车、
 *        不重置里程（里程仅供 K1 减速用）；IMU 校准与 K1 一致
 */
static void s_handle_k2_start(void)
{
    /* 1) IMU 陀螺仪校准（与 K1 一致） */
    MID_IMU_CalibrateGyro();
    /* 加载 K2 完整默认参数集（覆盖当前 g_param，含 PID/速度/使能） */
    MID_Param_LoadK2Defaults();
    /* 保存 K2 巡航速度（ramp 收敛目标），再加发车偏移 */
    s_cruise_vx = g_param.chassis_vx;
    /* 2) K2 慢发起步速度 = 巡航速度 + g_param.launch_offset（默认 -100） */
    g_param.chassis_vx = s_cruise_vx + g_param.launch_offset;

    /* 3) 重置紧急停车标志与窗口，但 K2 模式禁用紧急停车检测 */
    g_line_emergency_stopped = 0u;
    MID_Line_ResetEmergency();
    g_line_emergency_enable = 0U;   /* K2 模式：脱线不停车 */

    /* 4) 启动底盘控制 */
    MID_Chassis_Start();

    /* 5) 进入 K2 模式（s_running 同步置位，保持兼容） */
    s_running = 1u;
    s_launch_mode = MID_MODE_K2;

    /* 6) 蜂鸣器 1 响提示发车 */
    MID_Beep_Beep(1, 0.08f, 0.0f);
}

/* K1 按键事件处理：根据当前状态决定启动或停止 */
static void s_handle_k1_press(void)
{
    if (s_launch_mode == MID_MODE_IDLE) {
        /* 停止态 -> K1 快发 */
        s_handle_start();
    } else if (s_launch_mode == MID_MODE_K1) {
        /* K1 运行态 -> 停止 */
        s_handle_stop();
    }
    /* K2 模式下按 K1 忽略（跨模式不切换） */
}

/* K2 按键事件处理：停止态触发 K2 慢发，K2 运行态停止，K1 运行态忽略 */
static void s_handle_k2_press(void)
{
    if (s_launch_mode == MID_MODE_IDLE) {
        s_handle_k2_start();
    } else if (s_launch_mode == MID_MODE_K2) {
        s_handle_stop();
    }
    /* K1 模式下按 K2 忽略（跨模式不切换） */
}

/* 按键周期性扫描函数
 * 详细说明请参考 mid_key.h
 */
void MID_Key_Scan(void)
{
    /* 1) 限频：每 20ms 执行一次 LQ_Key_Scan */
    uint32_t now = APP_Scheduler_GetTickMs();
    if ((now - s_last_key_scan_ms) < 20) {
        return;
    }
    s_last_key_scan_ms = now;

    /* 2) K1 按键检测：若检测到按下（返回值为 1），则切换运行/停止状态 */
    if (LQ_Key_Scan(KEY1) == 1) {
        s_handle_k1_press();
    }

    /* 3) K0 按键预留（暂未定义功能）；K2 短按 -> K2 发车模式 toggle */
    (void)LQ_Key_Scan(KEY0);

    /* K2 短按：停止态触发 K2 慢发，K2 运行态停止，K1 运行态忽略 */
    if (LQ_Key_Scan(KEY2) == 1) {
        s_handle_k2_press();
    }
}