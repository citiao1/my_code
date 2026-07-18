#ifndef VEHICLE_TELEMETRY_H
#define VEHICLE_TELEMETRY_H

#include <stdint.h>

#include "board_io.h"
#include "vehicle_cascade_control.h"

/*
 * 应用层在发送前构造只读快照，遥测模块不保存第二份车辆状态。
 * 这样复位编码器、切换控制模式或重标定 IMU 后不会出现状态分叉。
 */
typedef struct
{
    uint32_t now_ms;
    int32_t max_test_speed_mm_s;
    uint16_t line_edge_target_min;
    uint8_t closed_loop_active;
    uint8_t line_mode_active;
    uint8_t track_color_mode;
    uint8_t local_line_running;
    uint8_t link_active;
    uint8_t yaw_control_enabled;
    uint8_t heading_control_enabled;
    uint8_t heading_hold_active;

    int32_t target_forward_mm_s;
    int32_t target_yaw_rate10;
    int32_t target_left_mm_s;
    int32_t target_right_mm_s;
    int32_t error_left_mm_s;
    int32_t error_right_mm_s;

    int32_t pid_left_kp;
    int32_t pid_left_ki;
    int32_t pid_left_kd;
    int32_t pid_right_kp;
    int32_t pid_right_ki;
    int32_t pid_right_kd;

    int32_t yaw_pid_kp_micro;
    int32_t yaw_pid_ki_micro;
    int32_t yaw_pid_kd_micro;
    int32_t yaw_pid_kff_micro;
    int32_t max_yaw_rate_dps;

    int32_t heading_pid_kp_milli;
    int32_t heading_pid_kd_milli;
    int32_t heading_pid_kff_milli;
    int32_t max_heading_rate_dps;
    float target_heading_deg;

    int32_t line_pid_kp_milli;
    int32_t line_pid_ki_milli;
    int32_t line_pid_kd_milli;
    int32_t line_diff_milli;
    int32_t effective_line_diff_milli;

    VehicleCascadeOutput control_output;
} VehicleTelemetrySnapshot;

void VehicleTelemetry_SendGrayCalibration(const VehicleTelemetrySnapshot *snapshot);
void VehicleTelemetry_SendKey(const VehicleTelemetrySnapshot *snapshot,
                              BoardKeyEvents events);
void VehicleTelemetry_SendMode(const VehicleTelemetrySnapshot *snapshot);
void VehicleTelemetry_SendLine(const VehicleTelemetrySnapshot *snapshot);
void VehicleTelemetry_SendSquare(const VehicleTelemetrySnapshot *snapshot);
void VehicleTelemetry_SendState(const VehicleTelemetrySnapshot *snapshot);
void VehicleTelemetry_SendSpeed(const VehicleTelemetrySnapshot *snapshot);
void VehicleTelemetry_SendDebug(void);

#endif
