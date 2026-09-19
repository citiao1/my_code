#include "app.h"

#include "battery.h"
#include "chassis_test.h"
#include "command.h"
#include "display.h"
#include "encoder.h"
#include "gyro.h"
#include "main.h"
#include "serial_dma.h"
#include "stm32f4xx_hal.h"
#include "telemetry.h"

#define APP_IMU_PERIOD_MS 10U

static uint32_t imu_tick;

void App_Init(void)
{
  uint8_t imu_ready;
  uint8_t encoder_ready;
  uint8_t motor_ready;
  uint8_t serial_ready;

  imu_ready = Gyro_Init();
  serial_ready = SerialDma_Init(Command_HandleLine);
  encoder_ready = Encoder_Init();
  Battery_Init();
  motor_ready = ChassisTest_Init();
  Display_Init();
  Telemetry_Init();
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

  SerialDma_Process();
  ChassisTest_Process();
  Battery_Process();
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
    Gyro_Process(dt_seconds,
                 (ChassisTest_IsMotorActive() == 0U) ? 1U : 0U);
    gyro = Gyro_GetSnapshot();
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin,
                      (gyro.connected != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }

  Display_Process();
  SerialDma_Process();
}
