/*******************************************************************************
 * @file    mid_pid.c
 * @brief   通用 PID 控制器实现 + 底盘控制实现
 *
 * @note    提供三种 PID 实现：
 *          - 位置式 PD（用于位置环）
 *          - 偏航角 PI（带积分限幅）
 *          - 增量式速度 PID（带抗饱和 Back‑Calculation）
 *
 *          通用保护措施：
 *          - 输出限幅：out_max
 *          - 积分限幅：imax（仅偏航 PI）
 *          - 微分一阶低通滤波，抑制高频噪声，滤波系数 D_FILTER_ALPHA 可调
 *          - 增量式 PID 的积分抗饱和（Back‑Calculation）：输出限幅时削减增量，
 *            防止积分饱和，使控制器快速退出饱和区
 *
 *          调用顺序（速度‑角度‑位置级联）：
 *          速度环 PI  →  偏航角 PI  →  位置式 PD
 *******************************************************************************/

#include "mid_pid.h"
#include <string.h>  /* memcpy */

/* 微分一阶低通滤波系数，范围 0~1，越小滤波越强。
 * 若控制周期为 5ms，alpha=0.5 对应截止频率约为 64Hz */
#define D_FILTER_ALPHA      (0.5f)

/* 增量式 PID 的 dt 下限，防止上层传入 0 或异常值导致计算错误 */
#define SPEED_PID_DT_FLOOR  (0.001f)

/* 全局参数结构体，存储当前激活的 PID 参数 */
MID_Param_t g_param;

/* -------------------- K1 模式默认参数 --------------------
 * K1 模式专用于“云台发射 + 陀螺仪补偿”场景。
 * 与 K2 的主要差异：
 *   - pid_pos.kp = 50.0f（K2 为 39.0f）
 *   - launch_offset = +100（发射偏移）
 *   - decel_end_offset = -120（减速终点偏移）
 *   - ramp_step = -1（减速斜坡）
 *   - enables 默认全为 1（启动后自动使能） */
static const MID_Param_t s_param_k1_default = {
    .pid_pos   = { .kp = 57.0f,  .kd = 2.1f, .out_max = 500.0f },
    .pid_yaw   = { .kp = 17.5f, .ki = 15.0f, .kd = 0.0f, .imax = 80.0f, .out_max = 300.0f },
    .pid_speed_l = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },
    .pid_speed_r = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },
    .chassis_vx  = 200.0f,
    .speed_max        = 550.0f,
    .gyro_offset      = 0.0f,
    .launch_offset    = 0.0f,    /* K1 发射偏移 = +100 */
    .decel_end_offset = -190.0f,   /* K1 减速终点偏移 = -120 */
    .ramp_step        = -3.15f,     /* K1 斜坡步进 = -1（减速） */
    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,
};

/* -------------------- K2 模式默认参数 --------------------
 * K2 模式专用于“普通底盘 + 传统控制”场景。
 * 参数值与原始 MID_Param_Init 保持一致，enables 默认全为 1。 */
static const MID_Param_t s_param_k2_default = {
    .pid_pos   = { .kp = 39.0f,  .kd = 0.0f, .out_max = 300.0f },
    .pid_yaw   = { .kp = 17.5f, .ki = 15.0f, .kd = 0.0f, .imax = 80.0f, .out_max = 300.0f },
    .pid_speed_l = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },
    .pid_speed_r = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },
    .chassis_vx  = 130.0f,
    .speed_max        = 550.0f,
    .gyro_offset      = 0.0f,
    .launch_offset    = -100.0f,   /* K2 发射偏移 = -100 */
    .decel_end_offset = 0.0f,      /* K2 不使用减速终点，设为 0 */
    .ramp_step        = 1.0f,      /* K2 斜坡步进 = +3（加速） */
    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,
};

/* 加载 K1 默认参数到全局结构体 g_param */
void MID_Param_LoadK1Defaults(void)
{
    memcpy(&g_param, &s_param_k1_default, sizeof(MID_Param_t));
}

/* 加载 K2 默认参数到全局结构体 g_param */
void MID_Param_LoadK2Defaults(void)
{
    memcpy(&g_param, &s_param_k2_default, sizeof(MID_Param_t));
}

/* 初始化所有参数为默认值（K2 模式），并将所有使能位清零（完全关闭控制）。
 * 注意：调用后会强制 enables=0，进入安全状态。 */
void MID_Param_Init(void)
{
    MID_Param_LoadK2Defaults();
    g_param.enable_pos   = 0;
    g_param.enable_yaw   = 0;
    g_param.enable_speed = 0;
}

/* -------------------- 辅助函数：数值限幅 -------------------- */
static float mid_pid_constrain(float amt, float low, float high)
{
    return (amt < low) ? low : ((amt > high) ? high : amt);
}

/* ============================================================
 *  位置式 PD 控制器
 *  用于位置环，输出为速度或角度指令
 * ============================================================ */
void MID_PID_Pos_Init(MID_PID_Pos_State_t *s)
{
    s->last_error  = 0.0f;
    s->last_deriv  = 0.0f;
    s->out         = 0.0f;
}

float MID_PID_Pos_Calc(const MID_PID_Pos_t *param, MID_PID_Pos_State_t *s,
                       float error, float dt_s)
{
    float p_out, d_out;
    float deriv;

    /* 比例项 */
    p_out = param->kp * error;

    /* 微分项（带一阶低通滤波） */
    if (dt_s > 1e-6f) {
        deriv = (error - s->last_error) / dt_s;
    } else {
        deriv = 0.0f;
    }
    s->last_deriv = s->last_deriv * (1.0f - D_FILTER_ALPHA) + deriv * D_FILTER_ALPHA;
    d_out = param->kd * s->last_deriv;

    s->last_error = error;

    /* 输出限幅，并加入一阶惯性平滑（0.5 * 上一帧输出 + 0.5 * 当前计算值） */
    s->out = (0.5f) * s->last_out + (0.5f) * mid_pid_constrain(p_out + d_out, -param->out_max, param->out_max);
    s->last_out = mid_pid_constrain(p_out + d_out, -param->out_max, param->out_max);

    return s->out;
}

/* ============================================================
 *  偏航角 PI 控制器（带积分限幅）
 *  用于陀螺仪角速度闭环，输入为目标角速度和实际角速度
 * ============================================================ */
void MID_PID_Yaw_Init(MID_PID_Yaw_State_t *s)
{
    s->integrator  = 0.0f;
    s->last_error  = 0.0f;
    s->last_deriv  = 0.0f;   /* 微分滤波状态 */
    s->out         = 0.0f;
}

float MID_PID_Yaw_Calc(const MID_PID_Yaw_t *param, MID_PID_Yaw_State_t *s,
                       float target_omega, float gyro_z, float dt_s)
{
    float error;
    float p_out, i_out, d_out;
    float deriv;

    error = target_omega - gyro_z;

    /* 比例项 */
    p_out = param->kp * error;

    /* 积分项（带积分限幅） */
    s->integrator += error * dt_s;
    s->integrator = mid_pid_constrain(s->integrator, -param->imax, param->imax);
    i_out = param->ki * s->integrator;

    /* 微分项（带一阶低通滤波），注意 dt<=0 时禁止微分 */
    if (dt_s > 1e-6f) {
        deriv = (error - s->last_error) / dt_s;
    } else {
        deriv = 0.0f;
    }
    s->last_deriv = s->last_deriv * (1.0f - D_FILTER_ALPHA) + deriv * D_FILTER_ALPHA;
    d_out = param->kd * s->last_deriv;

    s->last_error = error;

    /* 输出限幅 */
    s->out = mid_pid_constrain(p_out + i_out + d_out, -param->out_max, param->out_max);
    return s->out;
}

/* ============================================================
 *  增量式速度 PID（带 Back‑Calculation 抗积分饱和）
 *
 *  增量形式：
 *    du = Kp*(e[k] - e[k-1]) + Ki*e[k]*dt + Kd*(e[k] - 2*e[k-1] + e[k-2])/dt
 *    u[k] = u[k-1] + du
 *
 *  抗饱和（Back‑Calculation）：
 *    当 u[k] 超出限幅范围时，将超出部分从 du 中减去，从而限制积分项的累积，
 *    防止积分饱和，使控制器能快速退出饱和区。
 * ============================================================ */
void MID_PID_Speed_Init(MID_PID_Speed_State_t *s)
{
    s->prev_error = 0.0f;
    s->last_error = 0.0f;
    s->last_out   = 0.0f;
    s->out        = 0.0f;
}

float MID_PID_Speed_Calc(const MID_PID_Speed_t *param, MID_PID_Speed_State_t *s,
                         float target_speed, float actual_speed, float dt_s)
{
    float error;
    float p_inc, i_inc, d_inc;
    float candidate;
    float dt;

    error = target_speed - actual_speed;

    /* 防止 dt 过小或为零 */
    dt = (dt_s > SPEED_PID_DT_FLOOR) ? dt_s : SPEED_PID_DT_FLOOR;

    /* 增量计算：
     *   - 比例增量
     *   - 积分增量
     *   - 微分增量
     * 每项独立限幅，防止单次变化过大（保护电机和 PWM） */
    p_inc = param->kp * (error - s->last_error);
    i_inc = param->ki * error * dt;
    d_inc = (param->kd / dt) * (error - 2.0f * s->last_error + s->prev_error);

    /* 候选输出 = 上一帧输出 + 各增量之和（每项均限幅） */
    candidate = s->last_out
                + mid_pid_constrain(p_inc, -param->inc_max, param->inc_max)
                + mid_pid_constrain(i_inc, -param->inc_max, param->inc_max)
                + mid_pid_constrain(d_inc, -param->inc_max, param->inc_max);

    /* 最终输出限幅，并执行 Back‑Calculation（通过将限幅后的值赋给 s->out，
     * 而 last_out 保持限幅前的 candidate，实际控制量使用 s->out，
     * 这样下一帧计算时 last_out 为限幅后的值，等效于削减了超出部分的增量） */
    s->out = mid_pid_constrain(candidate, -param->out_max, param->out_max);

    /* 更新历史误差 */
    s->prev_error = s->last_error;
    s->last_error = error;
    s->last_out   = s->out;   /* last_out 记录限幅后的值（用于下一帧增量基准） */

    return s->out;
}