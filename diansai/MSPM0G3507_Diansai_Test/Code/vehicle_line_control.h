#ifndef VEHICLE_LINE_CONTROL_H
#define VEHICLE_LINE_CONTROL_H

#include <stdint.h>

#include "vehicle_gray.h"

/*
 * 灰度寻线方向外环配置。
 * error 的单位是百分比（-100..100），PID 原始输出沿用参考工程量级，
 * 再按 output_limit 映射到 target_yaw_limit_dps，避免把参考 IMU 原始单位
 * 直接当作本车的 degree/s。
 */
typedef struct
{
    float kp;
    float ki;
    float kd;
    float integral_limit;
    float output_limit;
    float target_yaw_limit_dps;
    float blind_turn_yaw_rate_dps;
    float filter_new_weight;
    float turn_memory_error_percent;
    uint16_t visible_sum_min;
    uint16_t gap_hold_ms;
    uint16_t blind_turn_ms;
    uint16_t turn_memory_ms;
    uint16_t edge_black_min;
    uint8_t reacquire_max_active;
    uint8_t reacquire_confirm_samples;
} VehicleLineConfig;

/*
 * WAIT_LINE 只用于刚启动且尚未看到线；GAP_HOLD 处理直线上的窄缝；
 * BLIND_TURN 只在刚记录到明确转向后进入；LOST 要求上层立即停机。
 */
typedef enum
{
    VEHICLE_LINE_WAIT_LINE = 0,
    VEHICLE_LINE_TRACKING,
    VEHICLE_LINE_GAP_HOLD,
    VEHICLE_LINE_BLIND_TURN,
    VEHICLE_LINE_LOST
} VehicleLineMode;

typedef struct
{
    float raw_error_percent;
    float filtered_error_percent;
    float integral;
    float previous_error_percent;
    float pid_output;
    float target_yaw_rate_dps;
    uint32_t lost_since_ms;
    uint32_t last_update_ms;
    uint32_t normalized_sum;
    uint32_t last_turn_seen_ms;
    uint32_t recovery_started_ms;
    int8_t last_turn_direction;
    int8_t blind_turn_direction;
    VehicleLineMode mode;
    uint8_t active_count;
    uint8_t reacquire_count;
    uint8_t visible;
    uint8_t lost;
    uint8_t has_seen_line;
} VehicleLineState;

/* 初始化并清空方向外环；调用后必须再配置参数。 */
void VehicleLine_Init(void);

/* 写入参数并清空 PID 历史，防止参数切换继承旧积分。 */
void VehicleLine_Configure(const VehicleLineConfig *config);

/* 清空 PID 和丢线历史，不改变配置。 */
void VehicleLine_Reset(void);

/*
 * 使用八路归一化灰度更新一次方向外环。返回 1 表示当前仍允许控制，
 * 返回 0 表示未标定或丢线超时，上层必须停机。
 */
uint8_t VehicleLine_Update(const VehicleGrayState *gray, uint32_t now_ms);

/* 返回模块内部只读状态快照。 */
const VehicleLineState *VehicleLine_GetState(void);

#endif
