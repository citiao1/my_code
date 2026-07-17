#ifndef VEHICLE_SQUARE_TEST_H
#define VEHICLE_SQUARE_TEST_H

#include <stdint.h>

/* 正方形测试阶段。数值保持与旧 diansai_test 的上位机协议一致。 */
typedef enum
{
    VEHICLE_SQUARE_IDLE = 0,
    VEHICLE_SQUARE_DRIVE = 1,
    VEHICLE_SQUARE_TURN = 2,
    VEHICLE_SQUARE_COMPLETE = 3,
    VEHICLE_SQUARE_ERROR = 4
} VehicleSquarePhase;

typedef struct
{
    int32_t start_left_count;
    int32_t start_right_count;
    int32_t progress_mm;
    int32_t remaining_mm;
    int32_t forward_command_mm_s;
    float baseline_heading_deg;
    float target_heading_deg;
    uint32_t phase_started_ms;
    uint32_t settle_started_ms;
    uint8_t leg;
    uint8_t active;
    uint8_t target_changed;
    uint8_t status_changed;
    VehicleSquarePhase phase;
} VehicleSquareState;

void VehicleSquare_Init(void);
uint8_t VehicleSquare_Start(uint32_t now_ms,
                            int32_t left_count,
                            int32_t right_count,
                            float heading_deg);
void VehicleSquare_Stop(VehicleSquarePhase phase);
void VehicleSquare_Update(uint32_t now_ms,
                          int32_t left_count,
                          int32_t right_count,
                          float left_speed_mm_s,
                          float right_speed_mm_s,
                          float heading_deg,
                          float yaw_rate_dps);
uint8_t VehicleSquare_TakeTargetChanged(void);
uint8_t VehicleSquare_TakeStatusChanged(void);
const VehicleSquareState *VehicleSquare_GetState(void);

#endif
