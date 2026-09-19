#include "chassis_test.h"

#include <string.h>

#include "stm32f4xx_hal.h"

#define CHASSIS_MAX_DUTY_PERCENT 30U
#define CHASSIS_MAX_JOG_MS       5000U
#define CHASSIS_MIN_JOG_MS       50U

static ChassisTestSnapshot chassis_state;
static uint32_t stop_tick;

uint8_t ChassisTest_Init(void)
{
  memset(&chassis_state, 0, sizeof(chassis_state));
  chassis_state.channel = MOTOR_CHANNEL_A;
  chassis_state.motor_ready = Motor_Init();
  return chassis_state.motor_ready;
}

void ChassisTest_Process(void)
{
  uint32_t now;

  if (chassis_state.active == 0U)
  {
    return;
  }
  now = HAL_GetTick();
  if ((int32_t)(now - stop_tick) >= 0)
  {
    ChassisTest_Stop();
  }
}

void ChassisTest_Arm(void)
{
  ChassisTest_Stop();
  if (chassis_state.motor_ready != 0U)
  {
    chassis_state.armed = 1U;
  }
}

void ChassisTest_Disarm(void)
{
  ChassisTest_Stop();
  chassis_state.armed = 0U;
}

void ChassisTest_Stop(void)
{
  Motor_StopAll();
  chassis_state.active = 0U;
  chassis_state.duty_percent = 0U;
  chassis_state.remaining_ms = 0U;
}

uint8_t ChassisTest_Jog(MotorChannel channel, int8_t direction,
                        uint8_t duty_percent, uint32_t duration_ms)
{
  if ((chassis_state.armed == 0U) ||
      ((uint32_t)channel >= (uint32_t)MOTOR_CHANNEL_COUNT) ||
      (duty_percent == 0U) || (duty_percent > CHASSIS_MAX_DUTY_PERCENT) ||
      (duration_ms < CHASSIS_MIN_JOG_MS) ||
      (duration_ms > CHASSIS_MAX_JOG_MS))
  {
    return 0U;
  }

  ChassisTest_Stop();
  if (Motor_Run(channel, direction, duty_percent) == 0U)
  {
    return 0U;
  }
  chassis_state.channel = channel;
  chassis_state.direction = (direction >= 0) ? 1 : -1;
  chassis_state.duty_percent = duty_percent;
  chassis_state.active = 1U;
  stop_tick = HAL_GetTick() + duration_ms;
  chassis_state.remaining_ms = duration_ms;
  return 1U;
}

uint8_t ChassisTest_Recover(MotorChannel channel)
{
  if ((chassis_state.armed == 0U) || (chassis_state.active != 0U))
  {
    return 0U;
  }
  return Motor_Recover(channel);
}

uint8_t ChassisTest_IsMotorActive(void)
{
  return chassis_state.active;
}

ChassisTestSnapshot ChassisTest_GetSnapshot(void)
{
  if (chassis_state.active != 0U)
  {
    uint32_t now = HAL_GetTick();
    chassis_state.remaining_ms =
        ((int32_t)(stop_tick - now) > 0) ? (stop_tick - now) : 0U;
  }
  return chassis_state;
}
