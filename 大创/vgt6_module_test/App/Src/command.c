#include "command.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "battery.h"
#include "chassis_test.h"
#include "encoder.h"
#include "gyro.h"
#include "serial_dma.h"
#include "telemetry.h"

#define COMMAND_SIZE 64U

static uint8_t Command_ParseMotor(char value, MotorChannel *channel)
{
  if ((value < 'A') || (value > 'D') || (channel == NULL))
  {
    return 0U;
  }
  *channel = (MotorChannel)(value - 'A');
  return 1U;
}

static void Command_JogDefault(MotorChannel channel, int8_t direction)
{
  if (ChassisTest_Jog(channel, direction, 10U, 1000U) == 0U)
  {
    SerialDma_Write("ERR,JOG,ARM_OR_RANGE\r\n");
  }
  else
  {
    char message[48];
    (void)snprintf(message, sizeof(message), "ACK,JOG,%c,%c,10,1000\r\n",
                   (char)('A' + (uint8_t)channel),
                   (direction >= 0) ? '+' : '-');
    SerialDma_Write(message);
  }
}

static void Command_Normalize(const char *source, char output[COMMAND_SIZE])
{
  size_t length;
  size_t index;

  while (isspace((unsigned char)*source) != 0)
  {
    ++source;
  }
  length = strlen(source);
  while ((length > 0U) &&
         (isspace((unsigned char)source[length - 1U]) != 0))
  {
    --length;
  }
  if (length >= COMMAND_SIZE)
  {
    length = COMMAND_SIZE - 1U;
  }
  for (index = 0U; index < length; ++index)
  {
    output[index] = (char)toupper((unsigned char)source[index]);
  }
  output[length] = '\0';
}

void Command_HandleLine(const char *line)
{
  char command[COMMAND_SIZE];
  GyroSnapshot gyro;

  if (line == NULL)
  {
    return;
  }
  Command_Normalize(line, command);
  if (command[0] == '\0')
  {
    return;
  }

  if (strcmp(command, "PING") == 0)
  {
    SerialDma_Write("PONG\r\n");
    return;
  }
  if (strcmp(command, "STATUS") == 0)
  {
    Telemetry_SendNow();
    return;
  }
  if (strcmp(command, "ARM") == 0)
  {
    ChassisTest_Arm();
    if (ChassisTest_GetSnapshot().armed != 0U)
    {
      SerialDma_Write("ACK,ARM,WHEELS_OFF_GROUND\r\n");
    }
    else
    {
      SerialDma_Write("ERR,ARM,MOTOR_INIT\r\n");
    }
    return;
  }
  if (strcmp(command, "DISARM") == 0)
  {
    ChassisTest_Disarm();
    SerialDma_Write("ACK,DISARM\r\n");
    return;
  }
  if (strcmp(command, "STOP") == 0)
  {
    ChassisTest_Stop();
    SerialDma_Write("ACK,STOP\r\n");
    return;
  }
  if ((strlen(command) == 2U) &&
      (command[0] >= 'A') && (command[0] <= 'D') &&
      ((command[1] == '+') || (command[1] == '-')))
  {
    Command_JogDefault((MotorChannel)(command[0] - 'A'),
                       (command[1] == '+') ? 1 : -1);
    return;
  }
  if (strncmp(command, "JOG,", 4U) == 0)
  {
    char channel_name;
    int speed_percent;
    unsigned long duration_ms;
    MotorChannel channel;

    if ((sscanf(command, "JOG,%c,%d,%lu", &channel_name,
                &speed_percent, &duration_ms) != 3) ||
        (Command_ParseMotor(channel_name, &channel) == 0U) ||
        (speed_percent == 0) || (speed_percent < -30) ||
        (speed_percent > 30) ||
        (ChassisTest_Jog(channel, (speed_percent > 0) ? 1 : -1,
                         (uint8_t)((speed_percent > 0) ? speed_percent :
                                   -speed_percent),
                         (uint32_t)duration_ms) == 0U))
    {
      SerialDma_Write("ERR,JOG,USE_JOG,A,+/-1..30,50..5000\r\n");
    }
    else
    {
      SerialDma_Write("ACK,JOG\r\n");
    }
    return;
  }
  if (strcmp(command, "ENC") == 0)
  {
    Telemetry_SendEncoders();
    return;
  }
  if (strcmp(command, "ENCZERO") == 0)
  {
    Encoder_Zero();
    SerialDma_Write("ACK,ENCZERO\r\n");
    return;
  }
  if (strncmp(command, "CPR,", 4U) == 0)
  {
    unsigned long cpr;
    if ((sscanf(command, "CPR,%lu", &cpr) != 1) ||
        (cpr > 20000UL) ||
        (Encoder_SetCountsPerRevolution((uint16_t)cpr) == 0U))
    {
      SerialDma_Write("ERR,CPR,RANGE_4_20000\r\n");
    }
    else
    {
      SerialDma_Write("ACK,CPR\r\n");
    }
    return;
  }
  if (strcmp(command, "BAT") == 0)
  {
    Telemetry_SendBattery();
    return;
  }
  if (strncmp(command, "RECOVER,", 8U) == 0)
  {
    MotorChannel channel;
    if ((strlen(command) != 9U) ||
        (Command_ParseMotor(command[8], &channel) == 0U) ||
        (ChassisTest_Recover(channel) == 0U))
    {
      SerialDma_Write("ERR,RECOVER,ARM_AND_STOP_FIRST\r\n");
    }
    else
    {
      SerialDma_Write("ACK,RECOVER\r\n");
    }
    return;
  }
  if ((strcmp(command, "CAL") == 0) ||
      (strcmp(command, "GYROCAL") == 0))
  {
    if (Gyro_StartCalibration() == 0U)
    {
      SerialDma_Write("ERR,IMU660RC_OFFLINE\r\n");
    }
    else
    {
      SerialDma_Write("ACK,CAL,KEEP_STILL\r\n");
    }
    return;
  }
  if ((strcmp(command, "ZERO") == 0) ||
      (strcmp(command, "YAWZERO") == 0))
  {
    Gyro_ZeroYaw();
    SerialDma_Write("ACK,ZERO\r\n");
    return;
  }
  if (strcmp(command, "WHO") == 0)
  {
    gyro = Gyro_GetSnapshot();
    {
      char message[64];
      (void)snprintf(message, sizeof(message),
                     "WHO,0x%02X,SPI=%u,HAL=%u,STATUS=0x%02X\r\n",
                     gyro.who_am_i, gyro.spi_transfer_ok,
                     gyro.spi_hal_status, gyro.sensor_status);
      SerialDma_Write(message);
    }
    return;
  }
  if ((strcmp(command, "HELP") == 0) || (strcmp(command, "?") == 0))
  {
    SerialDma_Write("CMD,ARM,DISARM,STOP,A+,A-,B+,B-,C+,C-,D+,D-\r\n");
    SerialDma_Write("CMD,JOG,A,+/-1..30,50..5000,ENC,ENCZERO,CPR,380\r\n");
    SerialDma_Write("CMD,BAT,RECOVER,A,STATUS,CAL,ZERO,WHO,PING,HELP\r\n");
    return;
  }

  SerialDma_Write("ERR,UNKNOWN_COMMAND\r\n");
}
