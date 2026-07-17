#include "vehicle_motor.h"

#include "include.h"

#define MOTOR_PWM_FREQUENCY_HZ 20000U

#define MOTOR_LEFT_TIMER       LQ_TIMERA_1
#define MOTOR_LEFT_IN1_PWM     LQ_TIMERA1_PWM_CH0_Pin_B_2
#define MOTOR_LEFT_IN2_PWM     LQ_TIMERA1_PWM_CH1_Pin_B_3

#define MOTOR_RIGHT_TIMER      LQ_TIMERG_0
#define MOTOR_RIGHT_IN1_PWM    LQ_TIMERG0_PWM_CH0_Pin_B_10
#define MOTOR_RIGHT_IN2_PWM    LQ_TIMERG0_PWM_CH1_Pin_B_11

static VehicleMotorState motor_state;
static int8_t left_previous_direction;
static int8_t right_previous_direction;

static int32_t ClampPercent(int32_t value)
{
    if (value < -100) return -100;
    if (value > 100) return 100;
    return value;
}

/* 初始化一个 H 桥占用的两个 PWM 通道，初始比较值为 0，确保上电不误转。 */
static void InitBridgeTimer(LQEnum_Timer_t timer,
                            LQEnum_PWM_Pin_t input1,
                            LQEnum_PWM_Pin_t input2)
{
    PWM_ConfigTypeDef cfg = LQ_PWM_CalcOptimal(timer, MOTOR_PWM_FREQUENCY_HZ);
    LQConfig_PWM_InitTypeDef_t pwm = {
        .DivideRatio = (DL_TIMER_CLOCK_DIVIDE)(cfg.DivideRatio - 1U),
        .Prescaler = cfg.Prescaler,
        .Period = cfg.Period,
        .PwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
        .startTimer = false,
    };

    LQ_TIMER_PWMInit(timer, &pwm);
    LQ_TIMER_EnablePWMChannel(input1);
    LQ_TIMER_EnablePWMChannel(input2);
    LQ_TIMER_PWMSetCaptureCompare(input1, 0U);
    LQ_TIMER_PWMSetCaptureCompare(input2, 0U);
    LQ_TIMER_PWM_Start(timer);
}

/*
 * AT8236 使用慢衰减：
 *   正转：IN1 恒高，IN2 输出互补占空比；
 *   反转：IN2 恒高，IN1 输出互补占空比。
 * 百分比仍表示实际驱动占空比，因此写入 PWM 的比较值需要使用 load-drive。
 * 只有方向真正翻转时才先关闭两臂 2 us，避免直通，同时不破坏连续同向 PWM。
 */
static void SetBridge(LQEnum_Timer_t timer,
                      LQEnum_PWM_Pin_t in1_pwm,
                      LQEnum_PWM_Pin_t in2_pwm,
                      int32_t percent,
                      int8_t *previous_direction)
{
    uint32_t load = LQ_TIMER_Regs[timer]->COUNTERREGS.LOAD;
    uint32_t drive_compare;
    uint32_t decay_compare;
    int8_t direction;

    percent = ClampPercent(percent);
    drive_compare = load * (uint32_t)abs(percent) / 100U;
    decay_compare = load - drive_compare;
    direction = (percent > 0) ? 1 : ((percent < 0) ? -1 : 0);

    if (percent == 0)
    {
        LQ_TIMER_PWMSetCaptureCompare(in1_pwm, 0U);
        LQ_TIMER_PWMSetCaptureCompare(in2_pwm, 0U);
        *previous_direction = 0;
        return;
    }

    if (*previous_direction != 0 && direction != *previous_direction)
    {
        LQ_TIMER_PWMSetCaptureCompare(in1_pwm, 0U);
        LQ_TIMER_PWMSetCaptureCompare(in2_pwm, 0U);
        delay_us(2);
    }

    if (percent > 0)
    {
        LQ_TIMER_PWMSetCaptureCompare(in1_pwm, load);
        LQ_TIMER_PWMSetCaptureCompare(in2_pwm, decay_compare);
    }
    else
    {
        LQ_TIMER_PWMSetCaptureCompare(in2_pwm, load);
        LQ_TIMER_PWMSetCaptureCompare(in1_pwm, decay_compare);
    }
    *previous_direction = direction;
}

void VehicleMotor_Init(void)
{
    memset(&motor_state, 0, sizeof(motor_state));
    left_previous_direction = 0;
    right_previous_direction = 0;
    InitBridgeTimer(MOTOR_LEFT_TIMER, MOTOR_LEFT_IN1_PWM, MOTOR_LEFT_IN2_PWM);
    InitBridgeTimer(MOTOR_RIGHT_TIMER, MOTOR_RIGHT_IN1_PWM, MOTOR_RIGHT_IN2_PWM);
    VehicleMotor_Stop();
}

void VehicleMotor_Set(int32_t left_percent, int32_t right_percent)
{
    motor_state.left_percent = (int8_t)ClampPercent(left_percent);
    motor_state.right_percent = (int8_t)ClampPercent(right_percent);
    SetBridge(MOTOR_LEFT_TIMER, MOTOR_LEFT_IN1_PWM, MOTOR_LEFT_IN2_PWM,
              motor_state.left_percent, &left_previous_direction);
    SetBridge(MOTOR_RIGHT_TIMER, MOTOR_RIGHT_IN1_PWM, MOTOR_RIGHT_IN2_PWM,
              motor_state.right_percent, &right_previous_direction);
}

void VehicleMotor_Stop(void)
{
    VehicleMotor_Set(0, 0);
}

const VehicleMotorState *VehicleMotor_GetState(void)
{
    return &motor_state;
}
