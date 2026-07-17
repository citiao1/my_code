#include "vehicle_cascade_control.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define VEHICLE_ALL_LOOPS (VEHICLE_LOOP_SPEED | VEHICLE_LOOP_YAW_RATE | VEHICLE_LOOP_HEADING)
#define VEHICLE_TARGET_EPSILON 0.01f
#define VEHICLE_SPEED_PID_PERIOD_S 0.01f

static float ClampFloat(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float WrapAngle(float angle_deg)
{
    /* 方向误差统一折算到最短旋转路径，避免跨越 +/-180 度时突然跳变 360 度。 */
    while (angle_deg > 180.0f) angle_deg -= 360.0f;
    while (angle_deg < -180.0f) angle_deg += 360.0f;
    return angle_deg;
}

static void ResetPid(VehiclePidState *state)
{
    state->integral = 0.0f;
    state->previous_error = 0.0f;
    state->previous_previous_error = 0.0f;
    state->output = 0.0f;
    state->target_direction = 0;
    state->previous_valid = 0U;
}

static float PidStep(const VehiclePidConfig *config,
                     VehiclePidState *state,
                     float error,
                     float feedforward,
                     float dt_s)
{
    float derivative = 0.0f;
    float candidate_integral;
    float unsaturated;
    float output;

    /*
     * 方向环和角速度环使用标准位置式 PID：
     * output = feedforward + Kp*e + Ki*integral(e) + Kd*de/dt。
     * 周期异常时回退到 10 ms，防止除零或一次长停顿放大积分/微分。
     */
    if (dt_s <= 0.0f || dt_s > 0.2f) dt_s = 0.01f;
    derivative = (error - state->previous_error) / dt_s;

    candidate_integral = state->integral + error * dt_s;
    candidate_integral = ClampFloat(candidate_integral,
                                    -fabsf(config->integral_limit),
                                    fabsf(config->integral_limit));
    unsaturated = feedforward + config->kp * error +
                  config->ki * candidate_integral + config->kd * derivative;
    output = ClampFloat(unsaturated,
                        -fabsf(config->output_limit),
                        fabsf(config->output_limit));

    /*
     * 条件积分抗饱和：未触及限幅时正常积分；已经饱和但当前误差会把输出
     * 拉回限幅区间时也允许积分，否则冻结积分，避免长时间累积后失控。
     */
    if (fabsf(output) < fabsf(config->output_limit) || output * error < 0.0f)
    {
        state->integral = candidate_integral;
    }
    state->previous_error = error;
    state->output = output;
    state->previous_valid = 1U;
    return output;
}

static int8_t DirectionOf(float value)
{
    if (value > VEHICLE_TARGET_EPSILON) return 1;
    if (value < -VEHICLE_TARGET_EPSILON) return -1;
    return 0;
}

static float IncrementalPidStep(const VehiclePidConfig *config,
                                VehiclePidState *state,
                                float target,
                                float error,
                                float feedforward,
                                float dt_s,
                                float *feedback_out)
{
    int8_t direction = DirectionOf(target);
    float delta;
    float feedback;
    float unsaturated;
    float output;
    float sample_scale;

    /* 目标正负翻转时清空累计 PWM，防止旧方向的积分拖慢换向。 */
    if (state->target_direction != 0 && direction != state->target_direction)
    {
        ResetPid(state);
    }
    state->target_direction = direction;

    if (dt_s <= 0.0f) dt_s = VEHICLE_SPEED_PID_PERIOD_S;
    sample_scale = ClampFloat(dt_s / VEHICLE_SPEED_PID_PERIOD_S, 0.5f, 5.0f);

    /*
     * 速度环完全沿用参考工程的 10 ms 离散增量式：
     * du = Kp*(e-e1) + Ki*e + Kd*(e-2e1+e2)。
     * 实际任务晚于 10 ms 时，Ki 按时间比例放大、Kd 按比例缩小，使参数含义
     * 不随偶发调度延迟变化；比例限制在 0.5..5，避免异常停顿造成突变。
     */
    delta = config->kp * (error - state->previous_error) +
            config->ki * sample_scale * error +
            config->kd / sample_scale *
                (error - 2.0f * state->previous_error +
                 state->previous_previous_error);
    feedback = ClampFloat(state->output + delta,
                          -fabsf(config->feedback_limit),
                          fabsf(config->feedback_limit));
    unsaturated = feedforward + feedback;
    output = ClampFloat(unsaturated,
                        -fabsf(config->output_limit),
                        fabsf(config->output_limit));

    /* 最终输出饱和时反算反馈累计量，避免前馈叠加后产生隐蔽积分饱和。 */
    if (output != unsaturated)
    {
        feedback = ClampFloat(output - feedforward,
                              -fabsf(config->feedback_limit),
                              fabsf(config->feedback_limit));
    }
    state->previous_previous_error = state->previous_error;
    state->previous_error = error;
    state->output = feedback;
    state->previous_valid = 1U;
    if (feedback_out != NULL) *feedback_out = feedback;
    return output;
}

static float MotorFeedforward(const VehicleMotorFeedforward *config,
                              float target_speed_mm_s)
{
    float magnitude;

    /* 零目标必须严格输出零，只有非零目标才叠加对应方向的已标定死区。 */
    if (fabsf(target_speed_mm_s) < VEHICLE_TARGET_EPSILON) return 0.0f;
    if (target_speed_mm_s > 0.0f)
    {
        magnitude = config->forward_percent_per_mm_s * fabsf(target_speed_mm_s);
        magnitude += config->forward_min_percent;
        return ClampFloat(magnitude, 0.0f, 100.0f);
    }
    magnitude = config->reverse_percent_per_mm_s * fabsf(target_speed_mm_s);
    magnitude += config->reverse_min_percent;
    return -ClampFloat(magnitude, 0.0f, 100.0f);
}

static VehiclePidConfig *GetPidConfig(VehicleCascadeControl *control, VehiclePidId pid_id)
{
    switch (pid_id)
    {
        case VEHICLE_PID_HEADING: return &control->heading_pid;
        case VEHICLE_PID_YAW_RATE: return &control->yaw_rate_pid;
        case VEHICLE_PID_SPEED_LEFT: return &control->speed_left_pid;
        case VEHICLE_PID_SPEED_RIGHT: return &control->speed_right_pid;
        default: return NULL;
    }
}

void VehicleCascade_Init(VehicleCascadeControl *control)
{
    if (control == NULL) return;
    memset(control, 0, sizeof(*control));
    control->max_speed_mm_s = 600.0f;
    control->max_yaw_rate_dps = 150.0f;
    control->max_wheel_correction_mm_s = 600.0f;
    control->heading_pid.output_limit = control->max_yaw_rate_dps;
    control->yaw_rate_pid.output_limit = control->max_wheel_correction_mm_s;
    control->speed_left_pid.output_limit = 100.0f;
    control->speed_right_pid.output_limit = 100.0f;
    /* 增益、前馈和使能位故意保持为零，必须由应用按当前车辆配置。 */
}

void VehicleCascade_Reset(VehicleCascadeControl *control)
{
    if (control == NULL) return;
    ResetPid(&control->heading_state);
    ResetPid(&control->yaw_rate_state);
    ResetPid(&control->speed_left_state);
    ResetPid(&control->speed_right_state);
}

void VehicleCascade_SetEnabledLoops(VehicleCascadeControl *control, uint8_t enabled_mask)
{
    uint8_t disabled;

    if (control == NULL) return;
    enabled_mask &= VEHICLE_ALL_LOOPS;
    disabled = control->enabled_mask & (uint8_t)~enabled_mask;
    control->enabled_mask = enabled_mask;
    if (disabled & VEHICLE_LOOP_HEADING) ResetPid(&control->heading_state);
    if (disabled & VEHICLE_LOOP_YAW_RATE) ResetPid(&control->yaw_rate_state);
    if (disabled & VEHICLE_LOOP_SPEED)
    {
        ResetPid(&control->speed_left_state);
        ResetPid(&control->speed_right_state);
    }
}

void VehicleCascade_ConfigurePid(VehicleCascadeControl *control,
                                 VehiclePidId pid_id,
                                 const VehiclePidConfig *config)
{
    VehiclePidConfig *target;

    if (control == NULL || config == NULL) return;
    target = GetPidConfig(control, pid_id);
    if (target == NULL) return;
    *target = *config;
    target->integral_limit = fabsf(target->integral_limit);
    target->feedback_limit = fabsf(target->feedback_limit);
    target->output_limit = fabsf(target->output_limit);
}

void VehicleCascade_SetMotorFeedforward(VehicleCascadeControl *control,
                                        const VehicleMotorFeedforward *left,
                                        const VehicleMotorFeedforward *right)
{
    if (control == NULL || left == NULL || right == NULL) return;
    control->left_feedforward = *left;
    control->right_feedforward = *right;
}

void VehicleCascade_Step(VehicleCascadeControl *control,
                         const VehicleCascadeInput *input,
                         float dt_s,
                         VehicleCascadeOutput *output)
{
    float target_yaw_rate;
    float yaw_feedforward = 0.0f;
    float correction;
    float correction_limit;
    float left_target;
    float right_target;
    float peak;

    if (control == NULL || input == NULL || output == NULL) return;
    memset(output, 0, sizeof(*output));

    /* 第一级：方向角环。关闭时直接透传上层给出的目标角速度。 */
    target_yaw_rate = input->requested_yaw_rate_dps;
    if (control->enabled_mask & VEHICLE_LOOP_HEADING)
    {
        target_yaw_rate = PidStep(&control->heading_pid,
                                  &control->heading_state,
                                  WrapAngle(input->requested_heading_deg -
                                            input->measured_heading_deg),
                                  0.0f,
                                  dt_s);
    }
    else
    {
        ResetPid(&control->heading_state);
    }
    target_yaw_rate = ClampFloat(target_yaw_rate,
                                 -fabsf(control->max_yaw_rate_dps),
                                 fabsf(control->max_yaw_rate_dps));

    /* 第二级：角速度环。输出是作用于左右轮的差速修正量，单位 mm/s。 */
    correction_limit = fabsf(control->max_wheel_correction_mm_s);
    if (input->wheel_correction_limit_valid &&
        input->max_wheel_correction_mm_s >= 0.0f &&
        input->max_wheel_correction_mm_s < correction_limit)
    {
        correction_limit = input->max_wheel_correction_mm_s;
    }
    correction = input->direct_wheel_correction_mm_s;
    if (control->enabled_mask & VEHICLE_LOOP_YAW_RATE)
    {
        VehiclePidConfig yaw_pid = control->yaw_rate_pid;

        /*
         * 寻线模式的差速比例需要在 PID 内部限幅，才能让条件积分抗饱和
         * 正确工作。DRV 模式不传本地限幅，因此行为与已调好的 V18 一致。
         */
        if (correction_limit < yaw_pid.output_limit)
        {
            yaw_pid.output_limit = correction_limit;
        }
        yaw_feedforward = control->yaw_rate_feedforward_mm_s_per_dps *
                          target_yaw_rate;
        correction = PidStep(&yaw_pid,
                             &control->yaw_rate_state,
                             target_yaw_rate - input->measured_yaw_rate_dps,
                             yaw_feedforward,
                             dt_s);
    }
    else
    {
        ResetPid(&control->yaw_rate_state);
    }
    correction = ClampFloat(correction, -correction_limit, correction_limit);

    /*
     * 差速混合约定：正角速度表示左转，因此左轮减、右轮加。
     * 若任一轮超出最大轮速，按相同比例缩放两轮以保持曲率不变。
     */
    left_target = input->requested_forward_speed_mm_s - correction;
    right_target = input->requested_forward_speed_mm_s + correction;
    peak = fmaxf(fabsf(left_target), fabsf(right_target));
    if (peak > fabsf(control->max_speed_mm_s) && peak > 0.0f)
    {
        left_target *= fabsf(control->max_speed_mm_s) / peak;
        right_target *= fabsf(control->max_speed_mm_s) / peak;
    }

    output->target_yaw_rate_dps = target_yaw_rate;
    output->yaw_error_dps = target_yaw_rate - input->measured_yaw_rate_dps;
    output->yaw_feedforward_mm_s = yaw_feedforward;
    output->yaw_pid_mm_s = (control->enabled_mask & VEHICLE_LOOP_YAW_RATE) ?
                         correction - yaw_feedforward : 0.0f;
    output->wheel_correction_mm_s = correction;
    output->target_left_speed_mm_s = left_target;
    output->target_right_speed_mm_s = right_target;

    /* 第三级：左右轮各自使用增量式速度 PID，最终输出电机百分比。 */
    if (!(control->enabled_mask & VEHICLE_LOOP_SPEED))
    {
        ResetPid(&control->speed_left_state);
        ResetPid(&control->speed_right_state);
        return;
    }

    if (fabsf(left_target) < VEHICLE_TARGET_EPSILON)
    {
        ResetPid(&control->speed_left_state);
        output->left_motor_percent = 0.0f;
    }
    else
    {
        output->left_feedforward_percent = MotorFeedforward(
            &control->left_feedforward, left_target);
        output->left_motor_percent = IncrementalPidStep(
            &control->speed_left_pid,
            &control->speed_left_state,
            left_target,
            left_target - input->measured_left_speed_mm_s,
            output->left_feedforward_percent,
            dt_s,
            &output->left_pid_percent);
    }

    if (fabsf(right_target) < VEHICLE_TARGET_EPSILON)
    {
        ResetPid(&control->speed_right_state);
        output->right_motor_percent = 0.0f;
    }
    else
    {
        output->right_feedforward_percent = MotorFeedforward(
            &control->right_feedforward, right_target);
        output->right_motor_percent = IncrementalPidStep(
            &control->speed_right_pid,
            &control->speed_right_state,
            right_target,
            right_target - input->measured_right_speed_mm_s,
            output->right_feedforward_percent,
            dt_s,
            &output->right_pid_percent);
    }
    output->motor_output_valid = 1U;
}
