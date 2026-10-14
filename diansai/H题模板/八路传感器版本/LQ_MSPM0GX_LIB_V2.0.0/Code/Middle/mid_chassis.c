/*******************************************************************************
 * @file    mid_chassis.c
 * @brief   底盘控制：位置环 + 偏航环 + 速度环 PID 实现
 *
 * @note    控制框架：每 5ms 执行一次
 *          位置环 PD(error_line)        -> target_omega (目标角速度)
 *          偏航环 PI(target, gyro)      -> target_diff (差速控制量)
 *          速度环 PI x2                 -> PWM 输出
 *
 *          速度合成公式：
 *          target_speed_l = chassis_vx + target_diff / 2
 *          target_speed_r = chassis_vx - target_diff / 2
 *
 *          每个 Step 函数都会检查 g_param.enable_xxx 标志以决定是否使能控制：
 *          - enable=0 时，控制环被旁路，目标值强制归零（或保持前一状态）
 *          - enable=1 时，控制环正常工作
 *          可通过 VOFA 上位机动态开关："en_pos=1" / "en_yaw=1" / "en_speed=1"
 *
 *          偏差来源：8 路电感取最大值对应的位置权重 -3.5 ~ +3.5，归一化为 -1~+1
 *          角速度限幅 + 电机保护
 *
 *          偏航环在 MID_Chassis_YawStep 中会利用 g_param.gyro_offset
 *          对原始陀螺仪值进行零偏校正，校正后的值存入 g_chassis.gyro_z_actual 供 VOFA 显示
 *******************************************************************************/

#include "mid_chassis.h"
#include "mid_key.h"            /* MID_Key_GetLaunchMode / MID_KEY_* 宏 */
#include "mid_encoder.h"        /* MID_Encoder_GetOdomMm 里程查询 */
#include "app_scheduler.h"        /* APP_Scheduler_GetTickMs 运行计时 */

/* 位置环软启动时长（ms）：K2 启动后前 N ms 内 pid_pos.kp 从 0 线性渐变到目标值，
 * 防止启动瞬间 e_line 非零被偏航环放大成左右轮反向（"突然转一下"）。
 * 仅对 K2 模式生效；K1 起步速度高（220mm/s）不需要软启动。
 */
#define POS_SOFTSTART_MS  (100.0f)

MID_Chassis_t g_chassis;   /* 全局底盘状态结构体 */

/* 运行计时状态（由 MID_Chassis_Start 启动，MID_Chassis_Stop 停止）
 * - s_run_timing=1 时 getter 返回 now - s_run_start_ms（实时增长）
 * - s_run_timing=0 时 getter 返回 s_run_elapsed_ms（停止时固化的时长）
 * - 上电默认全 0，未启动过则返回 0
 * - 再次 Start 会重置 s_run_start_ms，重新计时
 */
static uint32_t s_run_start_ms    = 0U;   /* 启动时刻 tick (ms) */
static uint32_t s_run_elapsed_ms  = 0U;   /* 停止时固化的运行时长 (ms) */
static uint8_t  s_run_timing      = 0U;   /* 1=正在计时，0=已停止 */

/* 初始化所有 PID 控制器状态及底盘变量
 * @note  调用后 g_chassis 所有成员清零，PID 历史数据重置
 *         PID 参数仍使用 g_param 中的配置，不修改
 */
void MID_Chassis_Init(void)
{
    MID_PID_Pos_Init(&g_chassis.pos);
    MID_PID_Yaw_Init(&g_chassis.yaw);
    MID_PID_Speed_Init(&g_chassis.speed_l);
    MID_PID_Speed_Init(&g_chassis.speed_r);

    g_chassis.target_omega    = 0.0f;
    g_chassis.target_diff     = 0.0f;
    g_chassis.target_speed_l  = 0.0f;
    g_chassis.target_speed_r  = 0.0f;
    g_chassis.pwm_l           = 0.0f;
    g_chassis.pwm_r           = 0.0f;
    g_chassis.gyro_z_actual   = 0.0f;
    g_chassis.yaw_deg         = 0.0f;
}

/* 位置环 PD 控制器：根据电感偏差计算目标角速度
 * @param error_line  归一化偏差（-1 ~ +1）
 * @param dt_s        控制周期（秒）
 * @输出  g_chassis.target_omega
 * @note  使能标志 g_param.enable_pos
 *        - enable_pos=1 时执行位置环 PD，输出 target_omega
 *        - enable_pos=0 时强制 target_omega = 0，VOFA 可显示 omega=0
 */
void MID_Chassis_PosStep(float error_line, float dt_s)
{
    if (g_param.enable_pos) {
        /* K2 模式位置环软启动：启动后前 POS_SOFTSTART_MS 内，
         * pid_pos.kp 从 0 线性渐变到目标值，防止启动瞬间 e_line 非零
         * 被偏航环放大成左右轮反向（"突然转一下"）。
         * K1 起步速度高（220mm/s），不启用软启动。 */
        float kp_scale = 1.0f;
        if (s_run_timing && MID_Key_GetLaunchMode() == MID_MODE_K2) {
            uint32_t elapsed_ms = APP_Scheduler_GetTickMs() - s_run_start_ms;
            if (elapsed_ms < POS_SOFTSTART_MS) {
                kp_scale = (float)elapsed_ms / POS_SOFTSTART_MS;
            }
        }
        /* 复制一份 PID 参数，应用软启动系数（不修改 g_param 原值，
         * 避免 VOFA 运行中读到的 kp 被临时修改） */
        MID_PID_Pos_t pos_param = g_param.pid_pos;
        pos_param.kp *= kp_scale;
        g_chassis.target_omega = MID_PID_Pos_Calc(
            &pos_param, &g_chassis.pos, error_line, dt_s);
    } else {
        /* 位置环旁路，目标角速度置零 */
        g_chassis.target_omega = 0.0f;
    }
}

/* 偏航环 PI 控制器：目标角速度与实测陀螺仪偏差 -> 差速控制量
 * @param target_omega  目标角速度 (deg/s)，通常来自位置环输出或 VOFA 给定
 * @param gyro_z        陀螺仪原始 Z 轴角速度 (deg/s)，未校正零偏
 * @param dt_s          控制周期（秒）
 * @输出  g_chassis.target_diff, target_speed_l, target_speed_r
 *
 * @note  1) 首先进行零偏校正：gyro_corrected = gyro_z - g_param.gyro_offset
 *        2) 校正后的值存入 g_chassis.gyro_z_actual，供 VOFA 显示
 *        3) 使能标志 g_param.enable_yaw：
 *           - enable_yaw=1 时执行偏航环 PI，输出 target_diff
 *           - enable_yaw=0 时 target_diff = 0
 *        4) 合成左右轮目标速度，并进行 speed_max 限幅
 */
void MID_Chassis_YawStep(float target_omega, float gyro_z, float dt_s)
{
    /* 1) 零偏校正 */
    float gyro_corrected = gyro_z - g_param.gyro_offset;

    /* 2) 保存校正后的角速度用于 VOFA 显示 */
    g_chassis.gyro_z_actual = gyro_corrected;

    /* 3) 偏航环 PI：目标角速度 - 实际角速度 -> 差速控制量 */
    if (g_param.enable_yaw) {
        g_chassis.target_diff = MID_PID_Yaw_Calc(
            &g_param.pid_yaw, &g_chassis.yaw,
            target_omega, gyro_corrected, dt_s);
    } else {
        g_chassis.target_diff = 0.0f;
    }

    /* 4) 速度合成：直线速度 + 差速/2 */
    g_chassis.target_speed_l = g_param.chassis_vx + g_chassis.target_diff * 0.4f;
    g_chassis.target_speed_r = g_param.chassis_vx - g_chassis.target_diff * 0.4f;

    /* 5) 速度限幅，防止电机过速或撞墙 */
    if (g_chassis.target_speed_l >  g_param.speed_max) g_chassis.target_speed_l =  g_param.speed_max;
    if (g_chassis.target_speed_l < -g_param.speed_max) g_chassis.target_speed_l = -g_param.speed_max;
    if (g_chassis.target_speed_r >  g_param.speed_max) g_chassis.target_speed_r =  g_param.speed_max;
    if (g_chassis.target_speed_r < -g_param.speed_max) g_chassis.target_speed_r = -g_param.speed_max;
}

/* 速度环 PI 控制器：左右轮分别控制，输出 PWM 占空比
 * @param target_speed_l  左轮目标速度 (m/s)
 * @param target_speed_r  右轮目标速度 (m/s)
 * @param speed_l         左轮实测速度 (m/s)
 * @param speed_r         右轮实测速度 (m/s)
 * @param dt_s            控制周期（秒）
 * @输出  g_chassis.pwm_l, pwm_r，并直接调用 MID_Motor_SetDuty 设置电机
 *
 * @note  使能标志 g_param.enable_speed
 *        - enable_speed=1 时执行速度环 PI，输出 PWM
 *        - enable_speed=0 时 PWM 强制为 0（电机停止）
 *        输出的 PWM 值会被限幅，但限幅在 PID 函数内部已处理，
 *        此处仅作二次保险，直接调用电机驱动接口
 */
void MID_Chassis_SpeedStep(float target_speed_l, float target_speed_r,
                           float speed_l, float speed_r, float dt_s)
{
    if (g_param.enable_speed) {
        /* 左右轮速度环 PI -> PWM */
        g_chassis.pwm_l = MID_PID_Speed_Calc(
            &g_param.pid_speed_l, &g_chassis.speed_l,
            target_speed_l, speed_l, dt_s);

        g_chassis.pwm_r = MID_PID_Speed_Calc(
            &g_param.pid_speed_r, &g_chassis.speed_r,
            target_speed_r, speed_r, dt_s);
    } else {
        /* 速度环旁路，PWM 置零（电机停止） */
        g_chassis.pwm_l = 0.0f;
        g_chassis.pwm_r = 0.0f;
    }

    /* 直接输出 PWM 到电机（PID 内部已限幅，此处不再重复限幅） */
    MID_Motor_SetDuty(MID_MOTOR_LEFT,  (int32_t)g_chassis.pwm_l);
    MID_Motor_SetDuty(MID_MOTOR_RIGHT, (int32_t)g_chassis.pwm_r);
}

/* 紧急停止：重置所有 PID 历史，停止电机
 * @note  相当于调用 MID_Chassis_Init() + MID_Motor_Stop()
 *        用于安全急停场景
 */
void MID_Chassis_EmergencyStop(void)
{
    MID_Chassis_Init();
    MID_Motor_Stop();
}

/* 启动底盘控制：默认使能偏航环和速度环，位置环禁用
 * @note  可通过 VOFA 后续动态调整 enable 标志
 *        此函数只设置标志，不重置 PID 历史
 */
void MID_Chassis_Start(void)
{
    g_param.enable_pos   = 1;   /* 位置环开启 */
    g_param.enable_yaw   = 1;   /* 偏航环开启 */
    g_param.enable_speed = 1;   /* 速度环开启 */

    /* 启动运行计时：记录启动时刻，置计时标志
     * 再次启动会重置 start_ms，重新计时 */
    s_run_start_ms = APP_Scheduler_GetTickMs();
    s_run_timing   = 1U;
}

/* 停止底盘控制：重置 PID 历史，关闭所有控制环，直线速度置零，强制停止电机
 * @note  安全停止函数，适用于停车或紧急情况
 *        1) 重置所有 PID 状态（防止积分饱和）
 *        2) 关闭所有使能标志
 *        3) 直线速度置零
 *        4) 强制电机停止
 */
void MID_Chassis_Stop(void)
{
    /* 1) 重置 PID 历史，防止下次启动时积分累积 */
    MID_Chassis_Init();

    /* 2) 关闭所有控制环 */
    g_param.enable_pos   = 0;
    g_param.enable_yaw   = 0;
    g_param.enable_speed = 0;

    /* 3) 直线速度置零 */
    g_param.chassis_vx   = 0.0f;

    /* 4) 强制电停止 */
    MID_Motor_Stop();

    /* 5) 停止运行计时：仅当正在计时才固化时长，避免重复 Stop 覆盖
     *    手动 K1 急停和 5 路传感器触发停止都走此函数，统一在此停止计时 */
    if (s_run_timing) {
        s_run_elapsed_ms = APP_Scheduler_GetTickMs() - s_run_start_ms;
        s_run_timing = 0U;
    }
}

/* 重置 PID 历史状态（不改变使能标志和参数）
 * @note  用于需要清除积分项但保持控制使能的情况
 *        实际上调用 MID_Chassis_Init()，它会重置所有内部状态
 */
void MID_Chassis_ResetPID(void)
{
    MID_Chassis_Init();
}

/* 获取当前运行时长（毫秒）
 * @return 运行时长 ms：
 *         - 正在计时（s_run_timing=1）：now - s_run_start_ms，实时增长
 *         - 已停止（s_run_timing=0）：s_run_elapsed_ms，停止时固化的值
 *         - 未启动过：0（上电默认）
 * @note  供 main.c OLED 显示调用；计时由 MID_Chassis_Start/Stop 自动维护
 *        main.c 每 50ms 调用一次刷新显示
 */
uint32_t MID_Chassis_GetRunTimeMs(void)
{
    if (s_run_timing) {
        return APP_Scheduler_GetTickMs() - s_run_start_ms;
    }
    return s_run_elapsed_ms;
}
/* 速度渐变步进：由调度器每 MID_KEY_RAMP_PERIOD_MS ms 调用一次
 * 根据当前发车模式调整 g_param.chassis_vx：
 * - K1 模式：里程达阈值后递减，减到 巡航速度-100 停止
 * - K2 模式：递增到 巡航速度 停止
 * - IDLE：不动作
 * @note  需要先调用 MID_Key_GetLaunchMode 获取模式，
 *        K1 减速依赖 MID_Encoder_GetOdomMm 里程反馈
 */
void MID_Chassis_RampStep(void)
{
    /* 仅在运行模式（K1/K2）下步进，IDLE 不动作 */
    MID_LaunchMode_t mode = MID_Key_GetLaunchMode();
    float cruise = MID_Key_GetCruiseVx();   /* 当前模式巡航速度（ramp 目标） */

    if (mode == MID_MODE_K1) {
        /* K1 模式：里程达阈值后递减，减到 巡航速度-100 停止 */
        float odom_mm = MID_Encoder_GetOdomMm();
        float decel_end = cruise + g_param.decel_end_offset;  /* 巡航速度 - 100 */
        if (odom_mm >= MID_KEY_K1_DECEL_THRESHOLD_MM) {
            if (g_param.chassis_vx > decel_end) {
                g_param.chassis_vx += g_param.ramp_step;
                if (g_param.chassis_vx < decel_end) {
                    g_param.chassis_vx = decel_end;  /* clamp 不低于终点 */
                }
            }
        }
    } else if (mode == MID_MODE_K2) {
        /* K2 模式：递增到 巡航速度 停止 */
        if (g_param.chassis_vx < cruise) {
            g_param.chassis_vx += g_param.ramp_step;
            if (g_param.chassis_vx > cruise) {
                g_param.chassis_vx = cruise;  /* clamp 不超过原目标 */
            }
        }
    }
    /* MID_MODE_IDLE: 不动作 */
}