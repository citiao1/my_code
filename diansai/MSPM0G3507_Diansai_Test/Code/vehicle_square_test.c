#include "vehicle_square_test.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define COUNTS_PER_METER                 1867
#define SQUARE_SIDE_MM                   1000
#define SQUARE_DISTANCE_TOLERANCE_MM       10
#define SQUARE_POSITION_KP               1.2f
#define SQUARE_MAX_DRIVE_SPEED_MM_S       200.0f
#define SQUARE_MIN_DRIVE_SPEED_MM_S        40.0f
#define SQUARE_DRIVE_SPEED_TOLERANCE_MM_S  25.0f
#define SQUARE_HEADING_TOLERANCE_DEG        1.0f
#define SQUARE_YAW_RATE_TOLERANCE_DPS       4.0f
#define SQUARE_SETTLE_MS                  400U
#define SQUARE_DRIVE_TIMEOUT_MS         30000U
#define SQUARE_TURN_TIMEOUT_MS          10000U

static VehicleSquareState square;

static float WrapAngle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static int32_t RoundFloat(float value)
{
    return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static void SetTarget(float target_heading_deg)
{
    square.target_heading_deg = WrapAngle(target_heading_deg);
    square.target_changed = 1U;
    square.status_changed = 1U;
}

static void BeginDrive(uint32_t now_ms,
                       int32_t left_count,
                       int32_t right_count)
{
    square.start_left_count = left_count;
    square.start_right_count = right_count;
    square.progress_mm = 0;
    square.remaining_mm = SQUARE_SIDE_MM;
    square.forward_command_mm_s = (int32_t)SQUARE_MAX_DRIVE_SPEED_MM_S;
    square.phase = VEHICLE_SQUARE_DRIVE;
    square.phase_started_ms = now_ms;
    square.settle_started_ms = 0U;
    SetTarget(square.baseline_heading_deg + 90.0f * (float)square.leg);
}

static void BeginTurn(uint32_t now_ms)
{
    square.forward_command_mm_s = 0;
    square.phase = VEHICLE_SQUARE_TURN;
    square.phase_started_ms = now_ms;
    square.settle_started_ms = 0U;
    /* 本车坐标中正角速度为左转，因此每次目标航向增加 90 度。 */
    SetTarget(square.baseline_heading_deg + 90.0f * (float)(square.leg + 1U));
}

void VehicleSquare_Init(void)
{
    memset(&square, 0, sizeof(square));
    square.phase = VEHICLE_SQUARE_IDLE;
}

uint8_t VehicleSquare_Start(uint32_t now_ms,
                            int32_t left_count,
                            int32_t right_count,
                            float heading_deg)
{
    if (square.active) return 1U;

    memset(&square, 0, sizeof(square));
    square.active = 1U;
    square.leg = 0U;
    square.baseline_heading_deg = WrapAngle(heading_deg);
    BeginDrive(now_ms, left_count, right_count);
    return 1U;
}

void VehicleSquare_Stop(VehicleSquarePhase phase)
{
    square.active = 0U;
    square.phase = phase;
    square.forward_command_mm_s = 0;
    square.target_changed = 0U;
    square.status_changed = 1U;
}

void VehicleSquare_Update(uint32_t now_ms,
                          int32_t left_count,
                          int32_t right_count,
                          float left_speed_mm_s,
                          float right_speed_mm_s,
                          float heading_deg,
                          float yaw_rate_dps)
{
    float heading_error;

    if (!square.active) return;
    heading_error = WrapAngle(square.target_heading_deg - heading_deg);

    if (square.phase == VEHICLE_SQUARE_DRIVE)
    {
        int32_t left_mm = (int32_t)(((int64_t)(left_count - square.start_left_count) *
                                     1000) / COUNTS_PER_METER);
        int32_t right_mm = (int32_t)(((int64_t)(right_count - square.start_right_count) *
                                      1000) / COUNTS_PER_METER);
        float linear_speed = 0.5f * (left_speed_mm_s + right_speed_mm_s);
        float desired_speed;

        square.progress_mm = (left_mm + right_mm) / 2;
        square.remaining_mm = SQUARE_SIDE_MM - square.progress_mm;
        desired_speed = SQUARE_POSITION_KP * (float)square.remaining_mm;
        if (desired_speed > SQUARE_MAX_DRIVE_SPEED_MM_S)
            desired_speed = SQUARE_MAX_DRIVE_SPEED_MM_S;
        if (desired_speed < -SQUARE_MAX_DRIVE_SPEED_MM_S)
            desired_speed = -SQUARE_MAX_DRIVE_SPEED_MM_S;

        if (abs(square.remaining_mm) <= SQUARE_DISTANCE_TOLERANCE_MM)
        {
            desired_speed = 0.0f;
        }
        else if (fabsf(desired_speed) < SQUARE_MIN_DRIVE_SPEED_MM_S)
        {
            desired_speed = square.remaining_mm > 0 ?
                            SQUARE_MIN_DRIVE_SPEED_MM_S :
                            -SQUARE_MIN_DRIVE_SPEED_MM_S;
        }
        square.forward_command_mm_s = RoundFloat(desired_speed);

        if (abs(square.remaining_mm) <= SQUARE_DISTANCE_TOLERANCE_MM &&
            fabsf(linear_speed) <= SQUARE_DRIVE_SPEED_TOLERANCE_MM_S &&
            fabsf(heading_error) <= SQUARE_HEADING_TOLERANCE_DEG &&
            fabsf(yaw_rate_dps) <= SQUARE_YAW_RATE_TOLERANCE_DPS)
        {
            if (square.settle_started_ms == 0U) square.settle_started_ms = now_ms;
            if (now_ms - square.settle_started_ms >= SQUARE_SETTLE_MS)
            {
                BeginTurn(now_ms);
            }
        }
        else
        {
            square.settle_started_ms = 0U;
        }

        if (now_ms - square.phase_started_ms > SQUARE_DRIVE_TIMEOUT_MS)
        {
            VehicleSquare_Stop(VEHICLE_SQUARE_ERROR);
        }
    }
    else if (square.phase == VEHICLE_SQUARE_TURN)
    {
        square.forward_command_mm_s = 0;
        if (fabsf(heading_error) <= SQUARE_HEADING_TOLERANCE_DEG &&
            fabsf(yaw_rate_dps) <= SQUARE_YAW_RATE_TOLERANCE_DPS)
        {
            if (square.settle_started_ms == 0U) square.settle_started_ms = now_ms;
            if (now_ms - square.settle_started_ms >= SQUARE_SETTLE_MS)
            {
                square.leg++;
                if (square.leg >= 4U)
                {
                    VehicleSquare_Stop(VEHICLE_SQUARE_COMPLETE);
                }
                else
                {
                    BeginDrive(now_ms, left_count, right_count);
                }
            }
        }
        else
        {
            square.settle_started_ms = 0U;
        }

        if (square.active &&
            now_ms - square.phase_started_ms > SQUARE_TURN_TIMEOUT_MS)
        {
            VehicleSquare_Stop(VEHICLE_SQUARE_ERROR);
        }
    }
}

uint8_t VehicleSquare_TakeTargetChanged(void)
{
    uint8_t changed = square.target_changed;
    square.target_changed = 0U;
    return changed;
}

uint8_t VehicleSquare_TakeStatusChanged(void)
{
    uint8_t changed = square.status_changed;
    square.status_changed = 0U;
    return changed;
}

const VehicleSquareState *VehicleSquare_GetState(void)
{
    return &square;
}
