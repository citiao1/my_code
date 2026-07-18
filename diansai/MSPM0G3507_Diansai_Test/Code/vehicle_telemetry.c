#include "vehicle_telemetry.h"

#include <stdio.h>

#include "vehicle_battery.h"
#include "vehicle_encoder.h"
#include "vehicle_gray.h"
#include "vehicle_imu.h"
#include "vehicle_line_control.h"
#include "vehicle_motor.h"
#include "vehicle_square_test.h"
#include "wheeltec_link.h"

static int32_t RoundFloat(float value)
{
    return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static void SendText(const char *text)
{
    (void)WheeltecLink_SendText(text);
}

void VehicleTelemetry_SendGrayCalibration(const VehicleTelemetrySnapshot *snapshot)
{
    const VehicleGrayState *gray = VehicleGray_GetState();
    char line[160];

    if (snapshot == NULL) return;
    snprintf(line, sizeof(line),
             "CAL,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
             gray->white[0], gray->white[1], gray->white[2], gray->white[3],
             gray->white[4], gray->white[5], gray->white[6], gray->white[7],
             gray->black[0], gray->black[1], gray->black[2], gray->black[3],
             gray->black[4], gray->black[5], gray->black[6], gray->black[7],
             (unsigned int)snapshot->track_color_mode);
    SendText(line);

    /* 归一化帧只在查询或重新标定时发送，避免占用常规 9600 波特率预算。 */
    snprintf(line, sizeof(line),
             "NRM,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
             (unsigned long)snapshot->now_ms,
             (unsigned int)gray->normalization_valid,
             gray->normalized[0], gray->normalized[1],
             gray->normalized[2], gray->normalized[3],
             gray->normalized[4], gray->normalized[5],
             gray->normalized[6], gray->normalized[7],
             (unsigned int)snapshot->track_color_mode);
    SendText(line);
}

void VehicleTelemetry_SendKey(const VehicleTelemetrySnapshot *snapshot,
                              BoardKeyEvents events)
{
    char line[96];

    if (snapshot == NULL) return;
    snprintf(line, sizeof(line), "KEY,%lu,%u,%u,%u,%u,%u\r\n",
             (unsigned long)snapshot->now_ms,
             (unsigned int)events.pressed_mask,
             (unsigned int)events.released_mask,
             (unsigned int)events.short_press_mask,
             (unsigned int)events.long_press_mask,
             (unsigned int)BoardIo_GetPressedMask());
    SendText(line);
}

void VehicleTelemetry_SendMode(const VehicleTelemetrySnapshot *snapshot)
{
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleBatteryState *battery = VehicleBattery_GetState();
    uint8_t switches;
    char line[96];

    if (snapshot == NULL) return;
    switches = BoardIo_GetSwitchDownMask();
    snprintf(line, sizeof(line), "MOD,%lu,%u,%u,%u,%u,%u,%u,%lu\r\n",
             (unsigned long)snapshot->now_ms,
             (unsigned int)snapshot->track_color_mode,
             (unsigned int)((switches >> BOARD_SWITCH_1) & 1U),
             (unsigned int)((switches >> BOARD_SWITCH_2) & 1U),
             (unsigned int)snapshot->local_line_running,
             (unsigned int)gray->white_valid,
             (unsigned int)gray->black_valid,
             (unsigned long)(battery->valid ? battery->voltage_mv : 0U));
    SendText(line);
}

void VehicleTelemetry_SendLine(const VehicleTelemetrySnapshot *snapshot)
{
    const VehicleLineState *line = VehicleLine_GetState();
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    uint32_t recovery_ms = 0U;
    char frame[192];

    if (snapshot == NULL) return;
    if (line->mode == VEHICLE_LINE_GAP_HOLD ||
        line->mode == VEHICLE_LINE_BLIND_TURN)
    {
        recovery_ms = snapshot->now_ms - line->recovery_started_ms;
    }
    snprintf(frame, sizeof(frame),
             "LIN,%lu,%u,%u,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%lu,%u,%lu,%u\r\n",
             (unsigned long)snapshot->now_ms,
             (unsigned int)snapshot->line_mode_active,
             (unsigned int)gray->normalization_valid,
             (unsigned int)line->visible,
             (unsigned int)line->lost,
             (long)RoundFloat(line->raw_error_percent * 10.0f),
             (long)RoundFloat(line->filtered_error_percent * 10.0f),
             (long)RoundFloat(line->pid_output),
             (long)RoundFloat(line->target_yaw_rate_dps * 10.0f),
             (long)RoundFloat(imu->yaw_rate_dps * 10.0f),
             (long)RoundFloat(snapshot->control_output.wheel_correction_mm_s),
             (long)snapshot->target_left_mm_s,
             (long)snapshot->target_right_mm_s,
             (long)snapshot->target_forward_mm_s,
             (long)snapshot->effective_line_diff_milli,
             (unsigned long)line->normalized_sum,
             (unsigned int)line->mode,
             (unsigned long)recovery_ms,
             (unsigned int)line->active_count);
    SendText(frame);
}

void VehicleTelemetry_SendSquare(const VehicleTelemetrySnapshot *snapshot)
{
    const VehicleSquareState *square = VehicleSquare_GetState();
    uint8_t displayed_leg;
    char frame[112];

    if (snapshot == NULL) return;
    displayed_leg = square->leg < 4U ? (uint8_t)(square->leg + 1U) : 4U;
    snprintf(frame, sizeof(frame),
             "SQR,%lu,%u,%u,%u,%ld,%ld,%ld\r\n",
             (unsigned long)snapshot->now_ms,
             (unsigned int)square->active,
             (unsigned int)square->phase,
             (unsigned int)displayed_leg,
             (long)square->progress_mm,
             (long)square->remaining_mm,
             (long)RoundFloat(square->target_heading_deg * 10.0f));
    SendText(frame);
}

void VehicleTelemetry_SendState(const VehicleTelemetrySnapshot *snapshot)
{
    const VehicleMotorState *motor = VehicleMotor_GetState();
    const VehicleEncoderState *encoder = VehicleEncoder_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    const VehicleGrayState *gray = VehicleGray_GetState();
    const VehicleBatteryState *battery = VehicleBattery_GetState();
    int32_t target_left;
    int32_t target_right;
    int32_t yaw_rate10;
    uint8_t active;
    char line[384];

    if (snapshot == NULL) return;
    target_left = snapshot->target_left_mm_s;
    target_right = snapshot->target_right_mm_s;
    yaw_rate10 = RoundFloat(imu->yaw_rate_dps * 10.0f);
    active = VehicleGray_CountLineChannels(snapshot->line_edge_target_min);
    if (!snapshot->closed_loop_active)
    {
        target_left = motor->left_percent * snapshot->max_test_speed_mm_s / 100;
        target_right = motor->right_percent * snapshot->max_test_speed_mm_s / 100;
    }

    /* TEL 和 STA 的字段位置是网页兼容协议，不得随意重排。 */
    snprintf(line, sizeof(line),
             "TEL,%lu,%u,%u,%d,%ld,%ld,%ld,%ld,%d,%d,%ld,%ld,%ld,%ld,%u,%u,%ld,%ld,%ld,%ld,%u,%u,%lu\r\n",
             (unsigned long)snapshot->now_ms,
             (unsigned int)((motor->left_percent != 0 ||
                             motor->right_percent != 0) ? 1U : 0U),
             (unsigned int)snapshot->link_active,
             imu->yaw10,
             (long)encoder->filtered_left_mm_s,
             (long)encoder->filtered_right_mm_s,
             (long)target_left,
             (long)target_right,
             motor->left_percent * 168,
             motor->right_percent * 168,
             (long)RoundFloat(snapshot->control_output.target_yaw_rate_dps * 10.0f),
             (long)yaw_rate10,
             (long)RoundFloat(snapshot->control_output.yaw_error_dps * 10.0f),
             (long)RoundFloat(snapshot->control_output.wheel_correction_mm_s),
             (unsigned int)snapshot->yaw_control_enabled,
             (unsigned int)imu->ok,
             (long)RoundFloat(snapshot->control_output.yaw_feedforward_mm_s),
             (long)RoundFloat(snapshot->target_heading_deg * 10.0f),
             (long)RoundFloat(snapshot->control_output.heading_error_deg * 10.0f),
             (long)RoundFloat(snapshot->control_output.heading_output_dps * 10.0f),
             (unsigned int)snapshot->heading_control_enabled,
             (unsigned int)snapshot->heading_hold_active,
             (unsigned long)(battery->valid ? battery->voltage_mv : 0U));
    SendText(line);

    snprintf(line, sizeof(line),
             "STA,%lu,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,0,0,0,0,0,0,0,0,30,%u,90,"
             "%u,%u,%u,%u,%u,%u,%u,%u,60,%u,%u,%u,900,500,"
             "%u,%u,%u,%u,%u,%u,%u,%u,%u,%ld,%ld,%ld,%ld,%ld,"
             "%ld,%ld,%ld,%ld,%u,%ld,%ld,%ld,%ld\r\n",
             (unsigned long)snapshot->now_ms,
             imu->pitch10,
             imu->roll10,
             (long)encoder->total_left,
             (long)encoder->total_right,
             (long)snapshot->pid_left_kp,
             (long)snapshot->pid_left_ki,
             (long)snapshot->pid_left_kd,
             (long)snapshot->pid_right_kp,
             (long)snapshot->pid_right_ki,
             (long)snapshot->pid_right_kd,
             (unsigned int)active,
             gray->raw[0], gray->raw[1], gray->raw[2], gray->raw[3],
             gray->raw[4], gray->raw[5], gray->raw[6], gray->raw[7],
             (unsigned int)active,
             (unsigned int)gray->white_valid,
             (unsigned int)gray->black_valid,
             gray->raw[0], gray->raw[1], gray->raw[2], gray->raw[3],
             gray->raw[4], gray->raw[5], gray->raw[6], gray->raw[7],
             (unsigned int)snapshot->yaw_control_enabled,
             (long)snapshot->max_yaw_rate_dps,
             (long)snapshot->yaw_pid_kp_micro,
             (long)snapshot->yaw_pid_ki_micro,
             (long)snapshot->yaw_pid_kd_micro,
             (long)snapshot->yaw_pid_kff_micro,
             (long)snapshot->line_pid_kp_milli,
             (long)snapshot->line_pid_ki_milli,
             (long)snapshot->line_pid_kd_milli,
             (long)snapshot->line_diff_milli,
             (unsigned int)snapshot->heading_control_enabled,
             (long)snapshot->max_heading_rate_dps,
             (long)snapshot->heading_pid_kp_milli,
             (long)snapshot->heading_pid_kd_milli,
             (long)snapshot->heading_pid_kff_milli);
    SendText(line);
}

void VehicleTelemetry_SendSpeed(const VehicleTelemetrySnapshot *snapshot)
{
    const VehicleMotorState *motor = VehicleMotor_GetState();
    const VehicleEncoderState *encoder = VehicleEncoder_GetState();
    const VehicleImuState *imu = VehicleImu_GetState();
    char line[448];

    if (snapshot == NULL) return;
    snprintf(line, sizeof(line),
             "SPD,%lu,1,%ld,%ld,%ld,%ld,%ld,%d,%ld,%ld,%ld,%ld,%ld,%d,"
             "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"
             "%u,%ld,%ld,%ld,%ld,%ld,"
             "%ld,%ld,%ld,%ld,%ld,%ld,%u,%u,%ld,%ld,%ld,%ld\r\n",
             (unsigned long)snapshot->now_ms,
             (long)snapshot->target_left_mm_s,
             (long)encoder->filtered_left_mm_s,
             (long)snapshot->error_left_mm_s,
             (long)RoundFloat(snapshot->control_output.left_feedforward_percent * 10.0f),
             (long)RoundFloat(snapshot->control_output.left_pid_percent * 10.0f),
             motor->left_percent,
             (long)snapshot->target_right_mm_s,
             (long)encoder->filtered_right_mm_s,
             (long)snapshot->error_right_mm_s,
             (long)RoundFloat(snapshot->control_output.right_feedforward_percent * 10.0f),
             (long)RoundFloat(snapshot->control_output.right_pid_percent * 10.0f),
             motor->right_percent,
             (long)snapshot->pid_left_kp,
             (long)snapshot->pid_left_ki,
             (long)snapshot->pid_left_kd,
             (long)snapshot->pid_right_kp,
             (long)snapshot->pid_right_ki,
             (long)snapshot->pid_right_kd,
             (long)RoundFloat(snapshot->control_output.target_yaw_rate_dps * 10.0f),
             (long)RoundFloat(imu->yaw_rate_dps * 10.0f),
             (long)RoundFloat(snapshot->control_output.yaw_error_dps * 10.0f),
             (long)RoundFloat(snapshot->control_output.yaw_feedforward_mm_s),
             (long)RoundFloat(snapshot->control_output.yaw_pid_mm_s),
             (long)RoundFloat(snapshot->control_output.wheel_correction_mm_s),
             (unsigned int)snapshot->yaw_control_enabled,
             (long)snapshot->max_yaw_rate_dps,
             (long)snapshot->yaw_pid_kp_micro,
             (long)snapshot->yaw_pid_ki_micro,
             (long)snapshot->yaw_pid_kd_micro,
             (long)snapshot->yaw_pid_kff_micro,
             (long)RoundFloat(snapshot->target_heading_deg * 10.0f),
             (long)RoundFloat(snapshot->control_output.heading_reference_deg * 10.0f),
             (long)RoundFloat(snapshot->control_output.heading_reference_rate_dps * 10.0f),
             (long)imu->yaw10,
             (long)RoundFloat(snapshot->control_output.heading_error_deg * 10.0f),
             (long)RoundFloat(snapshot->control_output.heading_output_dps * 10.0f),
             (unsigned int)snapshot->heading_control_enabled,
             (unsigned int)snapshot->heading_hold_active,
             (long)snapshot->max_heading_rate_dps,
             (long)snapshot->heading_pid_kp_milli,
             (long)snapshot->heading_pid_kd_milli,
             (long)snapshot->heading_pid_kff_milli);
    SendText(line);
}

void VehicleTelemetry_SendDebug(void)
{
    const VehicleImuState *imu = VehicleImu_GetState();
    char line[96];

    snprintf(line, sizeof(line), "DBG,%u,%d,%d,%d,%d,%d,%d,%u,%u\r\n",
             imu->id,
             imu->ax, imu->ay, imu->az,
             imu->gx, imu->gy, imu->gz,
             imu->mosi_pin_number,
             imu->miso_pin_number);
    SendText(line);
}
