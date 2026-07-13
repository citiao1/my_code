#include "vehicle_app.h"

#include "main.h"
#include "vehicle_battery.h"
#include "vehicle_comm.h"
#include "vehicle_config.h"
#include "vehicle_control.h"
#include "vehicle_display.h"
#include "vehicle_gray.h"
#include "vehicle_imu.h"
#include "vehicle_internal.h"
#include "vehicle_motor.h"

VehicleState state;
SpeedPidState pid_left;
SpeedPidState pid_right;
YawPidState pid_yaw;
HeadingPidState pid_heading;
SquareTestState square_test;

uint32_t last_command_ms;
uint32_t last_heading_ms;

static uint32_t last_control_ms;
static uint32_t last_oled_ms;
static uint32_t last_telemetry_ms;
static uint32_t last_status_ms;
static uint32_t last_mpu_retry_ms;

void Vehicle_Init(void)
{
  VehicleControl_InitDefaults();
  VehicleMotor_Init();
  VehicleDisplay_Init();
  VehicleDisplay_WriteLine(0, "C30D VEHICLE");
  VehicleDisplay_WriteLine(1, "CALIBRATING IMU");

  state.mpu_ok = VehicleImu_InitWithRetry(3U);
  VehicleControl_EnableDefaultLoops();
  VehicleBattery_Update();
  VehicleGray_Init();
  VehicleComm_Init();

  last_control_ms = HAL_GetTick();
  last_oled_ms = last_control_ms;
  last_telemetry_ms = last_control_ms;
  last_status_ms = last_control_ms;
  VehicleDisplay_Update();
}

void Vehicle_Loop(void)
{
  uint32_t now = HAL_GetTick();
  VehicleComm_Process();

  if (!state.mpu_ok && state.throttle == 0 && state.steering == 0 &&
      (now - last_mpu_retry_ms) >= MPU_RETRY_PERIOD_MS)
  {
    last_mpu_retry_ms = now;
    VehicleMotor_Stop();
    state.mpu_ok = VehicleImu_InitWithRetry(1U);
    if (state.mpu_ok) VehicleControl_EnableDefaultLoops();
    now = HAL_GetTick();
  }

  if ((now - last_control_ms) >= CONTROL_PERIOD_MS)
  {
    last_control_ms += CONTROL_PERIOD_MS;
    VehicleControl_Update(now);
  }
  if ((now - last_telemetry_ms) >= TELEMETRY_PERIOD_MS)
  {
    last_telemetry_ms = now;
    VehicleComm_SendTelemetry(now);
  }
  if ((now - last_status_ms) >= STATUS_PERIOD_MS && !VehicleComm_IsTxBusy())
  {
    last_status_ms = now;
    VehicleComm_SendStatus(now);
  }
  if ((now - last_oled_ms) >= OLED_PERIOD_MS)
  {
    last_oled_ms = now;
    VehicleDisplay_Update();
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  VehicleComm_RxCallback(huart);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  VehicleComm_TxCallback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  VehicleComm_ErrorCallback(huart);
}
