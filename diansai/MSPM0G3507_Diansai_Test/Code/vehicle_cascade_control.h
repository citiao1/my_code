#ifndef VEHICLE_CASCADE_CONTROL_H
#define VEHICLE_CASCADE_CONTROL_H

#include <stdint.h>

/*
 * 三个控制环可以独立按位使能。V23 在遥控直行保持和航向阶跃测试时启用
 * 方向角环；寻线仍直接给目标角速度，不经过方向角环。
 */
#define VEHICLE_LOOP_SPEED      (1U << 0)
#define VEHICLE_LOOP_YAW_RATE   (1U << 1)
#define VEHICLE_LOOP_HEADING    (1U << 2)

typedef enum
{
    VEHICLE_PID_HEADING = 0,
    VEHICLE_PID_YAW_RATE,
    VEHICLE_PID_SPEED_LEFT,
    VEHICLE_PID_SPEED_RIGHT
} VehiclePidId;

/*
 * 通用 PID 参数。参数的物理单位由所在控制环决定：
 * 方向环输出 deg/s；角速度环输出差速修正 mm/s；速度环输出 PWM 百分比。
 * integral_limit 限制误差积分，feedback_limit 限制纯反馈部分，output_limit
 * 限制叠加前馈后的最终输出。所有限幅传入负数时也会取绝对值。
 */
typedef struct
{
    float kp;
    float ki;
    float kd;
    float integral_limit;
    float feedback_limit;
    float output_limit;
} VehiclePidConfig;

/* 轮速目标到电机百分比的四向死区/斜率映射；全零即完全关闭速度前馈。 */
typedef struct
{
    float forward_min_percent;
    float reverse_min_percent;
    float forward_percent_per_mm_s;
    float reverse_percent_per_mm_s;
} VehicleMotorFeedforward;

/* PID 历史仅供控制器内部维护，切模式、停机和目标换向时会按规则清零。 */
typedef struct
{
    float integral;
    float previous_error;
    float previous_previous_error;
    float output;
    int8_t target_direction;
    uint8_t previous_valid;
} VehiclePidState;

/* 串级控制器实例：包含参数、限幅、使能掩码和四个环的历史状态。 */
typedef struct
{
    uint8_t enabled_mask;
    float max_speed_mm_s;
    float max_yaw_rate_dps;
    float max_wheel_correction_mm_s;
    float yaw_rate_feedforward_mm_s_per_dps;
    float heading_feedforward;
    float heading_period_s;
    float heading_elapsed_s;
    float heading_reference_deg;
    float heading_reference_rate_dps;
    float heading_correction_deadband_deg;
    float heading_min_correction_dps;
    float heading_correction_rate_gate_dps;
    VehiclePidConfig heading_pid;
    VehiclePidConfig yaw_rate_pid;
    VehiclePidConfig speed_left_pid;
    VehiclePidConfig speed_right_pid;
    VehicleMotorFeedforward left_feedforward;
    VehicleMotorFeedforward right_feedforward;
    VehiclePidState heading_state;
    VehiclePidState yaw_rate_state;
    VehiclePidState speed_left_state;
    VehiclePidState speed_right_state;
    uint8_t heading_reference_valid;
} VehicleCascadeControl;

/*
 * 单次计算输入。角度单位为 degree，角速度为 degree/s，全部线速度为 mm/s。
 * 左转角速度为正、右转为负。方向环关闭时 requested_heading_deg 不参与计算；
 * 角速度环关闭时 direct_wheel_correction_mm_s 直接进入左右轮差速混合。
 */
typedef struct
{
    float requested_heading_deg;
    float requested_yaw_rate_dps;
    float requested_forward_speed_mm_s;
    float direct_wheel_correction_mm_s;
    float measured_heading_deg;
    float measured_yaw_rate_dps;
    float measured_left_speed_mm_s;
    float measured_right_speed_mm_s;
    /* 寻线模式可按基础速度收紧差速；是否生效由下一字段明确指定。 */
    float max_wheel_correction_mm_s;
    uint8_t wheel_correction_limit_valid;
} VehicleCascadeInput;

/*
 * 单次计算输出。角速度 PID 将误差换算成左右轮差速修正 mm/s，随后与前进
 * 速度混合；左右速度增量 PID 再输出 -100..100 的电机百分比。
 * motor_output_valid=0 表示速度环关闭，上层不应把电机字段写入 H 桥。
 */
typedef struct
{
    float target_heading_deg;
    float heading_reference_deg;
    float heading_reference_rate_dps;
    float heading_error_deg;
    float heading_output_dps;
    float target_yaw_rate_dps;
    float yaw_error_dps;
    float yaw_feedforward_mm_s;
    float yaw_pid_mm_s;
    float wheel_correction_mm_s;
    float target_left_speed_mm_s;
    float target_right_speed_mm_s;
    float left_feedforward_percent;
    float right_feedforward_percent;
    float left_pid_percent;
    float right_pid_percent;
    float left_motor_percent;
    float right_motor_percent;
    uint8_t heading_active;
    uint8_t motor_output_valid;
} VehicleCascadeOutput;

/* 清零实例并写入保守的默认限幅；增益和所有使能位保持为零。 */
void VehicleCascade_Init(VehicleCascadeControl *control);

/* 清空四个 PID 的积分、误差历史和累计输出，不改变参数与使能位。 */
void VehicleCascade_Reset(VehicleCascadeControl *control);

/* 更新使能掩码；从开变关的控制环会立即清空自己的历史状态。 */
void VehicleCascade_SetEnabledLoops(VehicleCascadeControl *control,
                                    uint8_t enabled_mask);

/* 配置指定 PID；调用者负责按该控制环的物理单位换算增益。 */
void VehicleCascade_ConfigurePid(VehicleCascadeControl *control,
                                 VehiclePidId pid_id,
                                 const VehiclePidConfig *config);

/* 配置左右轮速度前馈映射；传入全零结构即可关闭死区映射。 */
void VehicleCascade_SetMotorFeedforward(VehicleCascadeControl *control,
                                        const VehicleMotorFeedforward *left,
                                        const VehicleMotorFeedforward *right);

/* 从当前实测航向重新起步参考角斜坡；切换目标或开始直行保持时调用。 */
void VehicleCascade_ResetHeadingReference(VehicleCascadeControl *control,
                                          float measured_heading_deg);

/*
 * 按“方向角 -> 角速度 -> 左右轮速度”的顺序计算一次串级控制。
 * dt_s 必须是本次真实周期，控制器内部会对异常值进行保护。
 */
void VehicleCascade_Step(VehicleCascadeControl *control,
                         const VehicleCascadeInput *input,
                         float dt_s,
                         VehicleCascadeOutput *output);

#endif
