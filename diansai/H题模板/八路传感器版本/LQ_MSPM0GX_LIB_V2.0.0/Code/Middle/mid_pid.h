#ifndef __MID_PID_H__
#define __MID_PID_H__

/*******************************************************************************
 * @file    mid_pid.h
 * @brief   通用PID算法 + 参数配置定义（中间层）
 *
 * @note    本文件提供与硬件平台无关的通用 PID 实现
 *          支持三种模式：PD / PI / PID，可根据应用灵活选择
 *
 *          标准控制器：
 *          - 位置式 PD : MID_PID_Pos_Calc
 *          - 偏航角 PI : MID_PID_Yaw_Calc
 *          - 速度环 PID: MID_PID_Speed_Calc（增量式 + 抗积分饱和）
 *
 *          通用保护机制：输出限幅、积分限幅、增量式限幅
 *          增量式带 back-calculation 抗积分饱和
 *******************************************************************************/

#include "include.h"

/* ============================================================
 *  PID 参数结构体：每个控制器一个结构体，统一在 g_param 中管理
 * ============================================================ */

/* 位置式 PD（将循线偏差 e_line 转为目标角速度 target_omega
 * 调用周期 5ms
 */
typedef struct {
    float kp;
    float kd;
    float out_max;        /* 输出限幅（目标速度限幅，度/秒） */
} MID_PID_Pos_t;

/* 偏航角 PID 位置式（将目标角速度 - gyro_z 转为转向控制量 target_diff
 * 调用周期 5ms
 * @note  PI -> PID 时可启用 kd 项
 *        D 项含义：Kd * d(target_omega - gyro_z) / dt
 *        D 项配合一阶低通滤波（位置控制器中 D_FILTER_ALPHA=0.5）再乘 Kd
 *        建议先调整好 kp/ki 后，再逐步增加 kd，从 0.1 开始尝试
 */
typedef struct {
    float kp;
    float ki;
    float kd;             /* 微分项 Kd*err_diff/dt，带一阶低通滤波 */
    float imax;           /* 积分限幅 */
    float out_max;        /* 输出限幅 (m/s) */
} MID_PID_Yaw_t;

/* 速度环增量式 PID（将目标速度 mm/s 转为 PWM
 * 调用周期 5ms，每帧更新 K_i*err*dt，微分 K_d*(err-2err+prev)/dt，累加到 last_out
 * 输出限幅 out_max=9000（90% 占空比）
 * 增量限幅 inc_max：P/I/D 各项每帧变化限制，单位 PWM
 *        当目标速度从 0 跳到 500mm/s 时，P 项 kp*Δerr 可达 7500，即
 *        每帧 PWM 在 5ms 内变化 75% 占空比，左右轮 + 1ms 时延正常安全
 *        设置 inc_max=600 后 P 项被限制为 600，I/D 同时也被限制为 600
 */
typedef struct {
    float kp;
    float ki;
    float kd;             /* 微分项 Kd*err_diff/dt */
    float out_max;        /* 输出限幅 PWM 占空比 (0~10000) */
    float inc_max;        /* 增量限幅，P/I/D 各项每帧 PWM 变化限制，建议 300~800 */
} MID_PID_Speed_t;


/* 全局可调参数结构体：通过 VOFA+ FireWater 协议直接在线修改对应参数值 */
typedef struct {
    /* 核心 PID */
    MID_PID_Pos_t   pid_pos;        /* 位置式 PD */
    MID_PID_Yaw_t   pid_yaw;        /* 偏航角 PI */
    MID_PID_Speed_t pid_speed_l;    /* 左轮速度环 PID */
    MID_PID_Speed_t pid_speed_r;    /* 右轮速度环 PID */

    /* 运动参数，单位统一为 mm/s，历史原因编码器单位为 mm/5ms，换算时注意 */
    float chassis_vx;               /* 当前目标速度 (mm/s) */
    float speed_max;                /* 速度最大值限制 (mm/s) */

    /* 速度渐变参数（K1/K2 默认集中填不同值，发车时随 LoadXxxDefaults 加载）*/
    float launch_offset;      /* 发车偏移：chassis_vx = 巡航速度 + launch_offset */
    float decel_end_offset;   /* 减速终点偏移：decel_end = 巡航速度 + decel_end_offset（仅 K1 用，K2 填 0）*/
    float ramp_step;          /* 渐变步进量（带符号：负=减速 K1，正=加速 K2，代码统一用 +=）*/

    /* 陀螺仪 */
    float gyro_offset;              /* 陀螺仪零漂 (度/秒)，静止时校准 */

    /* 环路使能（1=启用，0=关闭），上电时默认关闭所有环路 */
    uint8_t enable_pos;             /* 位置环使能 */
    uint8_t enable_yaw;             /* 偏航环使能 */
    uint8_t enable_speed;           /* 速度环使能 */
} MID_Param_t;

/* 全局参数实例，在源文件中 extern 声明 */
extern MID_Param_t g_param;

/* 初始化所有参数为默认值 */
void MID_Param_Init(void);

/* 加载 K1 模式默认参数集到 g_param（发车时调用，覆盖当前活动参数） */
void MID_Param_LoadK1Defaults(void);

/* 加载 K2 模式默认参数集到 g_param（发车时调用，覆盖当前活动参数） */
void MID_Param_LoadK2Defaults(void);

/* ============================================================
 *  PID 控制器状态结构体：每个控制器实例持有一个状态，保存历史信息
 * ============================================================ */

/* 位置式 PD 状态 */
typedef struct {
    float last_error;       /* 上一次误差 */
    float last_deriv;       /* 上一次微分值（滤波后） */
    float out;              /* 当前输出 */
    float last_out;         /* 上一次输出 */
} MID_PID_Pos_State_t;

/* 偏航角 PID 位置式状态
 * @note  虽是 PI 控制器，但保留 D 项，last_deriv 用于滤波状态存储
 */
typedef struct {
    float integrator;       /* 积分累加器 */
    float last_error;       /* 上一次误差 */
    float last_deriv;       /* 上一次微分值（一阶低通滤波后） */
    float out;              /* 当前输出 */
} MID_PID_Yaw_State_t;

/* 速度环增量式 PID 状态
 * 增量式需要保存历史误差项，以及当前帧输出 + 上一帧输出 */
typedef struct {
    float prev_error;       /* 前两次误差 e[k-2] */
    float last_error;       /* 上一次误差 e[k-1] */
    float last_out;         /* 上一次输出 u[k-1]（增量式累加基础） */
    float out;              /* 当前输出 */
} MID_PID_Speed_State_t;

/* ============================================================
 *  API
 * ============================================================ */

/* 初始化各状态，清零历史 */
void MID_PID_Pos_Init(MID_PID_Pos_State_t *s);
void MID_PID_Yaw_Init(MID_PID_Yaw_State_t *s);
void MID_PID_Speed_Init(MID_PID_Speed_State_t *s);

/* 位置式 PD 计算
 * @param param   PID 参数（kp, kd, out_max）
 * @param s       状态
 * @param error   偏差（循线取值/位置偏差/角度偏差等）
 * @param dt_s    时间步长（秒）
 * @return        目标角速度 (度/秒)
 */
float MID_PID_Pos_Calc(const MID_PID_Pos_t *param, MID_PID_Pos_State_t *s,
                       float error, float dt_s);

/* 偏航角 PI 计算
 * @param param        PID 参数（kp, ki, imax, out_max）
 * @param s            状态
 * @param target_omega 目标角速度 (度/秒)
 * @param gyro_z       实际角速度 (度/秒)
 * @param dt_s         时间步长（秒）
 * @return             目标转向量 (m/s)
 */
float MID_PID_Yaw_Calc(const MID_PID_Yaw_t *param, MID_PID_Yaw_State_t *s,
                       float target_omega, float gyro_z, float dt_s);

/* 速度环 PID 计算（增量式 + back-calculation 抗积分饱和）
 * @param param        PID 参数（kp, ki, kd, out_max, inc_max）
 * @param s            状态
 * @param target_speed 目标速度 (mm/s)
 * @param actual_speed 实际速度 (mm/s)
 * @param dt_s         时间步长（秒）
 * @return             PWM 占空比 (-10000 ~ +10000)
 */
float MID_PID_Speed_Calc(const MID_PID_Speed_t *param, MID_PID_Speed_State_t *s,
                         float target_speed, float actual_speed, float dt_s);

#endif