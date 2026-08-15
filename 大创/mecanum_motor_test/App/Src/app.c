#include "app.h"

#include "command.h"
#include "battery.h"
#include "display.h"
#include "gyro.h"
#include "serial_dma.h"
#include "telemetry.h"
#include "vehicle.h"

void App_Init(void)
{
  uint8_t vehicle_ready = Vehicle_Init();
  uint8_t battery_ready = Battery_Init();
  uint8_t serial_ready = SerialDma_Init(Command_HandleLine);

  Display_Init();
  Telemetry_Init();

  SerialDma_Write("BOOT,MECANUM_F407\r\n");
  SerialDma_Write("MAP,A=RR,B=LR,C=RF,D=LF\r\n");
  if (vehicle_ready == 0U)
  {
    SerialDma_Write("ERR,VEHICLE_INIT\r\n");
  }
  if (serial_ready == 0U)
  {
    SerialDma_Write("ERR,SERIAL_DMA_INIT\r\n");
  }
  if (battery_ready == 0U)
  {
    SerialDma_Write("ERR,BATTERY_ADC_INIT\r\n");
  }
  if (Gyro_GetSnapshot().connected == 0U)
  {
    SerialDma_Write("ERR,MPU6050_INIT\r\n");
  }
}

void App_Process(void)
{
  SerialDma_Process();
  Vehicle_Process();
  Battery_Process();
  Telemetry_Process();
  Display_Process();
  SerialDma_Process();
}
