#ifndef STEPPER_HOLD_H
#define STEPPER_HOLD_H

#include <stdbool.h>
#include <stdint.h>

/* Enable only while the mechanism is manually held at its physical zero. */
#define STEPPER_HOLD_CALIBRATE_ORIGIN_ON_BOOT 0

#define STEPPER_HOLD_MOTOR_ADDRESS       1U
#define STEPPER_HOLD_PULSES_PER_REV       3200UL
#define STEPPER_HOLD_TARGET_DEG           30.0f
#define STEPPER_HOLD_TARGET_DIRECTION     0U
#define STEPPER_HOLD_SPEED_RPM            180U
#define STEPPER_HOLD_ACCELERATION         40U
#define STEPPER_HOLD_POSITION_TOLERANCE   1.0f

#define STEPPER_HOLD_POWERUP_DELAY_MS     500UL
#define STEPPER_HOLD_COMMAND_GAP_MS       100UL
#define STEPPER_HOLD_POSITION_POLL_MS     200UL
#define STEPPER_HOLD_COMMS_TIMEOUT_MS     600UL
#define STEPPER_HOLD_HOME_TIMEOUT_MS      15000UL
#define STEPPER_HOLD_MOVE_TIMEOUT_MS      15000UL
#define STEPPER_HOLD_SETTLE_SAMPLES       2U
#define STEPPER_HOLD_ERROR_SAMPLES        3U
#define STEPPER_HOLD_MAX_RECOVERY_RETRIES 2U

typedef enum
{
    STEPPER_HOLD_BOOT_DELAY,
    STEPPER_HOLD_CALIBRATING_ORIGIN,
    STEPPER_HOLD_HOMING,
    STEPPER_HOLD_MOVING_TO_TARGET,
    STEPPER_HOLD_HOLDING,
    STEPPER_HOLD_FAULT
} StepperHoldState;

typedef enum
{
    STEPPER_HOLD_FAULT_NONE,
    STEPPER_HOLD_FAULT_INVALID_TARGET,
    STEPPER_HOLD_FAULT_COMMS_TIMEOUT,
    STEPPER_HOLD_FAULT_HOME_TIMEOUT,
    STEPPER_HOLD_FAULT_MOVE_TIMEOUT,
    STEPPER_HOLD_FAULT_POSITION_UNRECOVERABLE
} StepperHoldFault;

typedef struct
{
    StepperHoldState state;
    StepperHoldFault fault;
    float actual_deg;
    float target_deg;
    float error_deg;
    bool position_valid;
    uint8_t recovery_retries;
} StepperHoldInfo;

void StepperHold_Init(uint32_t now_ms);
void StepperHold_Update(uint32_t now_ms);
const StepperHoldInfo *StepperHold_GetInfo(void);

#endif
