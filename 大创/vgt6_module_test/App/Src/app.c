#include "app.h"
#include "attitude_control.h"

#include "battery.h"
#include "chassis_test.h"
#include "command.h"
#include "display.h"
#include "encoder.h"
#include "gyro.h"
#include "main.h"
#include "serial_dma.h"
#include "ps2.h"
#include "ps2_control.h"
#include "speed_control.h"
#include "stack_usage.h"
#include "stm32f4xx_hal.h"
#include "telemetry.h"
#include "tof.h"

#define APP_IMU_PERIOD_MS 10U

static uint32_t imu_tick;

void App_Init(void)
{
  uint8_t imu_ready;
  uint8_t encoder_ready;
  uint8_t motor_ready;
  uint8_t serial_ready;

  StackUsage_Init();
  imu_ready = Gyro_Init();
  serial_ready = SerialDma_Init(Command_HandleLine);
  encoder_ready = Encoder_Init();
  Battery_Init();
  motor_ready = ChassisTest_Init();
  SpeedControl_Init();
  AttitudeControl_Init();
  Ps2_Init();
  Ps2Control_Init();
  Display_Init();
  Telemetry_Init();
  Tof_Init();
  imu_tick = HAL_GetTick();

  /* No boot banner or periodic telemetry: the Bluetooth link only replies
   * after an explicit command. Use STATUS/HELP to request diagnostics. */
  (void)imu_ready;
  (void)serial_ready;
  (void)encoder_ready;
  (void)motor_ready;
}

void App_Process(void)
{
  uint32_t now;
  uint32_t elapsed;
  float dt_seconds;
  GyroSnapshot gyro;
  SpeedControlSnapshot speed;
  EncoderSnapshot enc;
  uint8_t stationary;
  uint32_t i;

  if (StackUsage_IsHealthy() == 0U)
  {
    ChassisTest_Disarm();
    Telemetry_SetPeriodic(0U);
    SerialDma_Process();
    return;
  }
  SerialDma_Process();
  Ps2_Process();
  Ps2Control_Process();
  ChassisTest_Process();
  Battery_Process();
  Telemetry_Process();
  now = HAL_GetTick();
  elapsed = now - imu_tick;
  if (elapsed >= APP_IMU_PERIOD_MS)
  {
    if (elapsed > 100U)
    {
      elapsed = APP_IMU_PERIOD_MS;
    }
    imu_tick = now;
    dt_seconds = (float)elapsed / 1000.0f;
    Encoder_Process(elapsed);
    speed = SpeedControl_GetSnapshot();
    enc = Encoder_GetSnapshot();
    stationary = ChassisTest_IsMotorActive() == 0U;
    for (i = 0; i < 4U; ++i)
      if (speed.target_rpm[i] || speed.output_percent[i] ||
          enc.rpm_x10[i] > 20 || enc.rpm_x10[i] < -20)
        stationary = 0U;
    Gyro_Process(dt_seconds, stationary);
    AttitudeControl_Process(elapsed);
    SpeedControl_Process(elapsed);
    gyro = Gyro_GetSnapshot();
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin,
                      (gyro.connected != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }

  Tof_Process();
  Display_Process();
  SerialDma_Process();
}
