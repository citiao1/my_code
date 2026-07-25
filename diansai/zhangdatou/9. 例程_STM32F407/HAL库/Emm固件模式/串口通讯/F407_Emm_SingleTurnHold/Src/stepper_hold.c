#include "stepper_hold.h"

#include "Emm_V5.h"
#include "usart.h"

#define EMM_POSITION_RESPONSE_CMD 0x36U
#define EMM_FRAME_END             0x6BU
#define EMM_ABSOLUTE_POSITION     1U
#define EMM_SINGLE_TURN_NEAREST   0U

typedef struct
{
    StepperHoldInfo info;
    uint32_t state_started_ms;
    uint32_t last_poll_ms;
    uint32_t position_request_ms;
    bool awaiting_position;
    bool home_command_sent;
    uint8_t settled_samples;
    uint8_t error_samples;
} StepperHoldController;

static StepperHoldController controller;

static bool Elapsed(uint32_t now_ms, uint32_t since_ms, uint32_t interval_ms)
{
    return (uint32_t)(now_ms - since_ms) >= interval_ms;
}

static float AbsoluteFloat(float value)
{
    return value < 0.0f ? -value : value;
}

static float WrapError(float actual_deg, float target_deg)
{
    float error = actual_deg - target_deg;

    while (error > 180.0f)
    {
        error -= 360.0f;
    }
    while (error < -180.0f)
    {
        error += 360.0f;
    }

    return error;
}

static void EnterState(StepperHoldState state, uint32_t now_ms)
{
    controller.info.state = state;
    controller.state_started_ms = now_ms;
    controller.settled_samples = 0U;
    controller.error_samples = 0U;
    controller.awaiting_position = false;
}

static void EnterFault(StepperHoldFault fault, uint32_t now_ms)
{
    if (controller.info.state != STEPPER_HOLD_FAULT)
    {
        Emm_V5_Stop_Now(STEPPER_HOLD_MOTOR_ADDRESS, false);
    }

    controller.info.fault = fault;
    EnterState(STEPPER_HOLD_FAULT, now_ms);
}

static bool ConsumePositionResponse(void)
{
    uint8_t frame[8];
    uint8_t length = 0U;
    uint32_t position_raw;

    __disable_irq();
    if (rxFrameFlag)
    {
        length = rxCount;
        if (length == sizeof(frame))
        {
            for (uint8_t index = 0U; index < sizeof(frame); ++index)
            {
                frame[index] = rxFrame[index];
            }
        }
        rxFrameFlag = false;
    }
    __enable_irq();

    if ((length != sizeof(frame)) ||
        (frame[0] != STEPPER_HOLD_MOTOR_ADDRESS) ||
        (frame[1] != EMM_POSITION_RESPONSE_CMD) ||
        (frame[7] != EMM_FRAME_END))
    {
        return false;
    }

    position_raw = ((uint32_t)frame[3] << 24) |
                   ((uint32_t)frame[4] << 16) |
                   ((uint32_t)frame[5] << 8) |
                   (uint32_t)frame[6];

    controller.info.actual_deg = ((float)position_raw * 360.0f) / 65536.0f;
    if (frame[2] != 0U)
    {
        controller.info.actual_deg = -controller.info.actual_deg;
    }

    controller.info.error_deg = WrapError(controller.info.actual_deg,
                                           controller.info.target_deg);
    controller.info.position_valid = true;
    controller.awaiting_position = false;
    return true;
}

static void RequestPosition(uint32_t now_ms)
{
    if (!controller.awaiting_position &&
        Elapsed(now_ms, controller.last_poll_ms, STEPPER_HOLD_POSITION_POLL_MS))
    {
        Emm_V5_Read_Sys_Params(STEPPER_HOLD_MOTOR_ADDRESS, S_CPOS);
        controller.last_poll_ms = now_ms;
        controller.position_request_ms = now_ms;
        controller.awaiting_position = true;
    }
}

static void CheckCommsTimeout(uint32_t now_ms)
{
    if (controller.awaiting_position &&
        Elapsed(now_ms, controller.position_request_ms,
                STEPPER_HOLD_COMMS_TIMEOUT_MS))
    {
        EnterFault(STEPPER_HOLD_FAULT_COMMS_TIMEOUT, now_ms);
    }
}

static uint32_t TargetPulses(void)
{
    float pulses = (controller.info.target_deg *
                    (float)STEPPER_HOLD_PULSES_PER_REV) / 360.0f;

    return (uint32_t)(pulses + 0.5f);
}

static void StartTargetMove(uint32_t now_ms)
{
    Emm_V5_Pos_Control(STEPPER_HOLD_MOTOR_ADDRESS,
                        STEPPER_HOLD_TARGET_DIRECTION,
                        STEPPER_HOLD_SPEED_RPM,
                        STEPPER_HOLD_ACCELERATION,
                        TargetPulses(),
                        EMM_ABSOLUTE_POSITION,
                        false);
    EnterState(STEPPER_HOLD_MOVING_TO_TARGET, now_ms);
    controller.last_poll_ms = now_ms;
}

void StepperHold_Init(uint32_t now_ms)
{
    controller.info.state = STEPPER_HOLD_BOOT_DELAY;
    controller.info.fault = STEPPER_HOLD_FAULT_NONE;
    controller.info.actual_deg = 0.0f;
    controller.info.target_deg = STEPPER_HOLD_TARGET_DEG;
    controller.info.error_deg = 0.0f;
    controller.info.position_valid = false;
    controller.info.recovery_retries = 0U;
    controller.state_started_ms = now_ms;
    controller.last_poll_ms = now_ms;
    controller.position_request_ms = now_ms;
    controller.awaiting_position = false;
    controller.home_command_sent = false;
    controller.settled_samples = 0U;
    controller.error_samples = 0U;

    if ((controller.info.target_deg < 0.0f) ||
        (controller.info.target_deg >= 360.0f))
    {
        EnterFault(STEPPER_HOLD_FAULT_INVALID_TARGET, now_ms);
    }
}

void StepperHold_Update(uint32_t now_ms)
{
    bool received_position = ConsumePositionResponse();

    if (controller.info.state == STEPPER_HOLD_FAULT)
    {
        return;
    }

    switch (controller.info.state)
    {
        case STEPPER_HOLD_BOOT_DELAY:
            if (Elapsed(now_ms, controller.state_started_ms,
                        STEPPER_HOLD_POWERUP_DELAY_MS))
            {
                Emm_V5_En_Control(STEPPER_HOLD_MOTOR_ADDRESS, true, false);
                EnterState(STEPPER_HOLD_CALIBRATING_ORIGIN, now_ms);
            }
            break;

        case STEPPER_HOLD_CALIBRATING_ORIGIN:
            if (Elapsed(now_ms, controller.state_started_ms,
                        STEPPER_HOLD_COMMAND_GAP_MS))
            {
#if STEPPER_HOLD_CALIBRATE_ORIGIN_ON_BOOT
                Emm_V5_Origin_Set_O(STEPPER_HOLD_MOTOR_ADDRESS, true);
                controller.home_command_sent = false;
#else
                Emm_V5_Origin_Trigger_Return(STEPPER_HOLD_MOTOR_ADDRESS,
                                               EMM_SINGLE_TURN_NEAREST, false);
                controller.home_command_sent = true;
#endif
                EnterState(STEPPER_HOLD_HOMING, now_ms);
                controller.last_poll_ms = now_ms;
            }
            break;

        case STEPPER_HOLD_HOMING:
            if (Elapsed(now_ms, controller.state_started_ms,
                        STEPPER_HOLD_HOME_TIMEOUT_MS))
            {
                EnterFault(STEPPER_HOLD_FAULT_HOME_TIMEOUT, now_ms);
                break;
            }

#if STEPPER_HOLD_CALIBRATE_ORIGIN_ON_BOOT
            if (!controller.home_command_sent &&
                Elapsed(now_ms, controller.state_started_ms,
                        STEPPER_HOLD_COMMAND_GAP_MS))
            {
                Emm_V5_Origin_Trigger_Return(STEPPER_HOLD_MOTOR_ADDRESS,
                                               EMM_SINGLE_TURN_NEAREST, false);
                controller.home_command_sent = true;
                controller.last_poll_ms = now_ms;
            }
#endif

            RequestPosition(now_ms);
            if (received_position &&
                (AbsoluteFloat(controller.info.actual_deg) <=
                 STEPPER_HOLD_POSITION_TOLERANCE))
            {
                ++controller.settled_samples;
                if (controller.settled_samples >= STEPPER_HOLD_SETTLE_SAMPLES)
                {
                    StartTargetMove(now_ms);
                }
            }
            else if (received_position)
            {
                controller.settled_samples = 0U;
            }
            CheckCommsTimeout(now_ms);
            break;

        case STEPPER_HOLD_MOVING_TO_TARGET:
            if (Elapsed(now_ms, controller.state_started_ms,
                        STEPPER_HOLD_MOVE_TIMEOUT_MS))
            {
                if (controller.info.recovery_retries <
                    STEPPER_HOLD_MAX_RECOVERY_RETRIES)
                {
                    ++controller.info.recovery_retries;
                    StartTargetMove(now_ms);
                }
                else
                {
                    EnterFault(STEPPER_HOLD_FAULT_MOVE_TIMEOUT, now_ms);
                }
                break;
            }

            RequestPosition(now_ms);
            if (received_position &&
                (AbsoluteFloat(controller.info.error_deg) <=
                 STEPPER_HOLD_POSITION_TOLERANCE))
            {
                ++controller.settled_samples;
                if (controller.settled_samples >= STEPPER_HOLD_SETTLE_SAMPLES)
                {
                    EnterState(STEPPER_HOLD_HOLDING, now_ms);
                    controller.last_poll_ms = now_ms;
                }
            }
            else if (received_position)
            {
                controller.settled_samples = 0U;
            }
            CheckCommsTimeout(now_ms);
            break;

        case STEPPER_HOLD_HOLDING:
            RequestPosition(now_ms);
            if (received_position)
            {
                if (AbsoluteFloat(controller.info.error_deg) >
                    STEPPER_HOLD_POSITION_TOLERANCE)
                {
                    ++controller.error_samples;
                    if (controller.error_samples >=
                        STEPPER_HOLD_ERROR_SAMPLES)
                    {
                        if (controller.info.recovery_retries <
                            STEPPER_HOLD_MAX_RECOVERY_RETRIES)
                        {
                            ++controller.info.recovery_retries;
                            StartTargetMove(now_ms);
                        }
                        else
                        {
                            EnterFault(STEPPER_HOLD_FAULT_POSITION_UNRECOVERABLE,
                                       now_ms);
                        }
                    }
                }
                else
                {
                    controller.error_samples = 0U;
                    controller.info.recovery_retries = 0U;
                }
            }
            CheckCommsTimeout(now_ms);
            break;

        default:
            EnterFault(STEPPER_HOLD_FAULT_COMMS_TIMEOUT, now_ms);
            break;
    }
}

const StepperHoldInfo *StepperHold_GetInfo(void)
{
    return &controller.info;
}
