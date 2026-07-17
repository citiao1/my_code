#include "vehicle_line_control.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define LINE_DEFAULT_PERIOD_S 0.020f

/*
 * 假定 G0 位于车辆左侧、G7 位于右侧。目标线落到左侧时误差为正，
 * 对应本车“左转角速度为正”的坐标约定。首次架起车测试若符号相反，
 * 只需整体翻转这组权重，不能通过给 PID 填负 Kp 来掩盖接线方向。
 */
static const int8_t channel_weights[VEHICLE_GRAY_CHANNELS] = {
    7, 5, 3, 1, -1, -3, -5, -7,
};

static VehicleLineConfig line_config;
static VehicleLineState line_state;

static float ClampFloat(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static void ResetPidHistory(void)
{
    line_state.integral = 0.0f;
    line_state.previous_error_percent = 0.0f;
    line_state.pid_output = 0.0f;
    line_state.target_yaw_rate_dps = 0.0f;
}

static uint8_t FinishAsLost(void)
{
    line_state.lost = 1U;
    line_state.mode = VEHICLE_LINE_LOST;
    ResetPidHistory();
    return 0U;
}

/*
 * 丢线后先根据最近的边缘/大偏差记录选择恢复方式。没有可靠方向时只保持
 * 旧目标穿过窄缝；方向记录足够新时才允许盲转，避免直线断线后随意拐弯。
 */
static void BeginRecovery(uint32_t now_ms)
{
    line_state.recovery_started_ms = now_ms;
    line_state.lost_since_ms = now_ms;
    line_state.reacquire_count = 0U;
    if (line_state.last_turn_direction != 0 &&
        now_ms - line_state.last_turn_seen_ms <= line_config.turn_memory_ms)
    {
        line_state.mode = VEHICLE_LINE_BLIND_TURN;
        line_state.blind_turn_direction = line_state.last_turn_direction;
    }
    else
    {
        line_state.mode = VEHICLE_LINE_GAP_HOLD;
        line_state.blind_turn_direction = 0;
    }
}

static uint8_t ContinueRecovery(uint32_t now_ms)
{
    uint32_t timeout_ms = (line_state.mode == VEHICLE_LINE_BLIND_TURN) ?
                          line_config.blind_turn_ms : line_config.gap_hold_ms;

    if (now_ms - line_state.recovery_started_ms >= timeout_ms)
    {
        return FinishAsLost();
    }

    line_state.lost = 0U;
    if (line_state.mode == VEHICLE_LINE_BLIND_TURN)
    {
        line_state.pid_output =
            (float)line_state.blind_turn_direction * line_config.output_limit;
        line_state.target_yaw_rate_dps =
            (float)line_state.blind_turn_direction *
            line_config.blind_turn_yaw_rate_dps;
    }
    /* GAP_HOLD 保留最后一次 PID 输出和目标角速度，不在无反馈时继续积分。 */
    return 1U;
}

void VehicleLine_Init(void)
{
    memset(&line_config, 0, sizeof(line_config));
    memset(&line_state, 0, sizeof(line_state));
}

void VehicleLine_Configure(const VehicleLineConfig *config)
{
    if (config == NULL) return;
    line_config = *config;
    line_config.integral_limit = fabsf(line_config.integral_limit);
    line_config.output_limit = fabsf(line_config.output_limit);
    line_config.target_yaw_limit_dps = fabsf(line_config.target_yaw_limit_dps);
    line_config.blind_turn_yaw_rate_dps =
        fabsf(line_config.blind_turn_yaw_rate_dps);
    line_config.filter_new_weight =
        ClampFloat(line_config.filter_new_weight, 0.0f, 1.0f);
    line_config.turn_memory_error_percent =
        ClampFloat(fabsf(line_config.turn_memory_error_percent), 0.0f, 100.0f);
    if (line_config.reacquire_confirm_samples == 0U)
    {
        line_config.reacquire_confirm_samples = 1U;
    }
    VehicleLine_Reset();
}

void VehicleLine_Reset(void)
{
    memset(&line_state, 0, sizeof(line_state));
}

uint8_t VehicleLine_Update(const VehicleGrayState *gray, uint32_t now_ms)
{
    int32_t weighted_sum = 0;
    uint32_t sum = 0U;
    uint8_t channel;
    uint8_t active_count = 0U;
    uint8_t reacquired;
    int8_t turn_direction = 0;
    float error;
    float derivative;
    float candidate_integral;
    float unsaturated;
    float output;
    float dt_s;
    uint32_t elapsed_ms;

    if (gray == NULL || !gray->normalization_valid ||
        line_config.output_limit <= 0.0f)
    {
        line_state.visible = 0U;
        return FinishAsLost();
    }

    for (channel = 0U; channel < VEHICLE_GRAY_CHANNELS; channel++)
    {
        sum += gray->normalized[channel];
        weighted_sum += (int32_t)gray->normalized[channel] *
                        (int32_t)channel_weights[channel];
        if (gray->normalized[channel] >= line_config.edge_line_min)
        {
            active_count++;
        }
    }
    line_state.normalized_sum = sum;
    line_state.active_count = active_count;

    if (sum < line_config.visible_sum_min)
    {
        line_state.visible = 0U;
        line_state.reacquire_count = 0U;
        if (!line_state.has_seen_line)
        {
            return FinishAsLost();
        }
        if (line_state.mode != VEHICLE_LINE_GAP_HOLD &&
            line_state.mode != VEHICLE_LINE_BLIND_TURN)
        {
            BeginRecovery(now_ms);
        }
        return ContinueRecovery(now_ms);
    }

    line_state.visible = 1U;
    line_state.lost = 0U;
    line_state.has_seen_line = 1U;

    /* 权重最大绝对值为 7，因此除以 sum*7 后天然落在 -1..1。 */
    error = (float)weighted_sum * 100.0f / ((float)sum * 7.0f);
    error = ClampFloat(error, -100.0f, 100.0f);
    line_state.raw_error_percent = error;

    /*
     * 盲转时遇到五路以上同时检测到目标线，通常仍在直角交叉区域而不是已经
     * 对准新线。继续转到目标线收窄，并连续确认两次，防止一帧噪声过早退出。
     */
    if (line_state.mode == VEHICLE_LINE_BLIND_TURN)
    {
        if (active_count == 0U ||
            active_count > line_config.reacquire_max_active)
        {
            line_state.reacquire_count = 0U;
            return ContinueRecovery(now_ms);
        }
        if (line_state.reacquire_count < 255U)
        {
            line_state.reacquire_count++;
        }
        if (line_state.reacquire_count < line_config.reacquire_confirm_samples)
        {
            return ContinueRecovery(now_ms);
        }
    }

    reacquired = (line_state.mode == VEHICLE_LINE_GAP_HOLD ||
                  line_state.mode == VEHICLE_LINE_BLIND_TURN) ? 1U : 0U;
    line_state.mode = VEHICLE_LINE_TRACKING;
    line_state.lost_since_ms = 0U;
    line_state.recovery_started_ms = 0U;
    line_state.reacquire_count = 0U;

    if (line_state.last_update_ms == 0U || reacquired)
    {
        line_state.filtered_error_percent = error;
        line_state.integral = 0.0f;
        line_state.previous_error_percent = error;
    }
    else
    {
        line_state.filtered_error_percent =
            (1.0f - line_config.filter_new_weight) *
                line_state.filtered_error_percent +
            line_config.filter_new_weight * error;
    }

    elapsed_ms = (line_state.last_update_ms == 0U) ? 20U :
                 (now_ms - line_state.last_update_ms);
    line_state.last_update_ms = now_ms;
    if (elapsed_ms == 0U || elapsed_ms > 100U) dt_s = LINE_DEFAULT_PERIOD_S;
    else dt_s = (float)elapsed_ms / 1000.0f;

    /*
     * 保留参考工程的离散微分形式 Kd*(e-e1)，使 400/0/120 可直接作为
     * 起始量级；积分按真实秒数累计，并做独立限幅。
     */
    derivative = reacquired ? 0.0f :
                 line_state.filtered_error_percent -
                 line_state.previous_error_percent;
    candidate_integral = line_state.integral +
                         line_state.filtered_error_percent * dt_s;
    candidate_integral = ClampFloat(candidate_integral,
                                    -line_config.integral_limit,
                                    line_config.integral_limit);
    unsaturated = line_config.kp * line_state.filtered_error_percent +
                  line_config.ki * candidate_integral +
                  line_config.kd * derivative;
    output = ClampFloat(unsaturated,
                        -line_config.output_limit,
                        line_config.output_limit);

    /* 与输出同向饱和时冻结积分；误差开始把输出拉回时允许积分释放。 */
    if (fabsf(output) < line_config.output_limit ||
        output * line_state.filtered_error_percent < 0.0f)
    {
        line_state.integral = candidate_integral;
    }
    line_state.previous_error_percent = line_state.filtered_error_percent;
    line_state.pid_output = output;
    line_state.target_yaw_rate_dps =
        output / line_config.output_limit * line_config.target_yaw_limit_dps;

    /* G0/G7 边缘目标线优先；否则只有足够大的偏差才刷新短期转向记忆。 */
    if (gray->normalized[0] >= line_config.edge_line_min &&
        gray->normalized[VEHICLE_GRAY_CHANNELS - 1U] < line_config.edge_line_min)
    {
        turn_direction = 1;
    }
    else if (gray->normalized[VEHICLE_GRAY_CHANNELS - 1U] >=
                 line_config.edge_line_min &&
             gray->normalized[0] < line_config.edge_line_min)
    {
        turn_direction = -1;
    }
    else if (fabsf(error) >= line_config.turn_memory_error_percent)
    {
        turn_direction = (error > 0.0f) ? 1 : -1;
    }
    if (turn_direction != 0)
    {
        line_state.last_turn_direction = turn_direction;
        line_state.last_turn_seen_ms = now_ms;
    }
    return 1U;
}

const VehicleLineState *VehicleLine_GetState(void)
{
    return &line_state;
}
