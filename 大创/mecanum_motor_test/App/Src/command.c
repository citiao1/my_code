#include "command.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoder.h"
#include "gyro.h"
#include "heading_control.h"
#include "serial_dma.h"
#include "speed_control.h"
#include "telemetry.h"
#include "vehicle.h"
#include "yaw_rate_control.h"

#define COMMAND_SIZE 64U

static int32_t Command_GainToMilli(float gain)
{
  return (int32_t)(gain * 1000.0f + 0.5f);
}

static void Command_SendPidAck(void)
{
  char message[96];
  SpeedControlConfig config = SpeedControl_GetConfig();

  (void)snprintf(message, sizeof(message), "ACK,PID,%ld,%ld,%ld,%u,%u,%u\r\n",
                 (long)Command_GainToMilli(config.kp),
                 (long)Command_GainToMilli(config.ki),
                 (long)Command_GainToMilli(config.kd),
                 config.output_limit, config.integral_limit, config.enabled);
  SerialDma_Write(message);
}

static void Command_SendYawPidAck(void)
{
  char message[96];
  YawRateControlSnapshot yaw = YawRateControl_GetSnapshot();

  (void)snprintf(message, sizeof(message), "ACK,YAWPID,%ld,%ld,%ld,%u\r\n",
                 (long)Command_GainToMilli(yaw.kp),
                 (long)Command_GainToMilli(yaw.ki),
                 (long)Command_GainToMilli(yaw.kff), yaw.enabled);
  SerialDma_Write(message);
}

static void Command_SendHeadingAck(void)
{
  char message[96];
  HeadingControlSnapshot heading = HeadingControl_GetSnapshot();

  (void)snprintf(message, sizeof(message), "ACK,HEADPID,%ld,%ld,%ld,%u\r\n",
                 (long)Command_GainToMilli(heading.kp),
                 (long)Command_GainToMilli(heading.kd),
                 (long)(heading.max_rate_dps + 0.5f),
                 heading.enabled);
  SerialDma_Write(message);
}

static void Command_SendMoveAck(const char *command, uint8_t accepted)
{
  char message[32];

  if (accepted == 0U)
  {
    SerialDma_Write("ERR,MOVE_REJECTED\r\n");
    return;
  }

  (void)snprintf(message, sizeof(message), "ACK,MOVE,%s\r\n", command);
  SerialDma_Write(message);
}

static void Command_HandleSpeed(const char *command)
{
  char message[32];
  char *end;
  long speed = strtol(command + 6, &end, 10);

  if ((*end != '\0') || (speed < 0L) || (speed > 65535L) ||
      (Vehicle_SetSpeed((uint16_t)speed) == 0U))
  {
    SerialDma_Write("ERR,SPEED_RANGE,5,120\r\n");
    return;
  }

  (void)snprintf(message, sizeof(message), "ACK,SPEED,%u\r\n", Vehicle_GetSpeed());
  SerialDma_Write(message);
}

static void Command_HandleDrive(const char *command)
{
  char message[64];
  char trailing;
  int forward_rpm;
  int left_rpm;
  int yaw_rate_dps;
  uint16_t limit = Vehicle_GetSpeed();

  if (sscanf(command, "DRV,%d,%d,%d%c", &forward_rpm, &left_rpm,
             &yaw_rate_dps, &trailing) != 3)
  {
    SerialDma_Write("ERR,DRV_FORMAT,FORWARD_RPM,LEFT_RPM,YAW_DPS\r\n");
    return;
  }
  if ((forward_rpm > (int)limit) || (forward_rpm < -(int)limit) ||
      (left_rpm > (int)limit) || (left_rpm < -(int)limit) ||
      (yaw_rate_dps > (int)limit) || (yaw_rate_dps < -(int)limit))
  {
    (void)snprintf(message, sizeof(message), "ERR,DRV_RANGE,%u\r\n", limit);
    SerialDma_Write(message);
    return;
  }
  if (Vehicle_Drive((int16_t)forward_rpm, (int16_t)left_rpm,
                    (int16_t)yaw_rate_dps) == 0U)
  {
    SerialDma_Write("ERR,DRV_REJECTED\r\n");
    return;
  }

  (void)snprintf(message, sizeof(message), "ACK,DRV,%d,%d,%d\r\n",
                 forward_rpm, left_rpm, yaw_rate_dps);
  SerialDma_Write(message);
}

static void Command_HandleHeadingStep(const char *command)
{
  char message[40];
  char trailing;
  int delta_deg;

  if (sscanf(command, "HEADSTEP,%d%c", &delta_deg, &trailing) != 1)
  {
    SerialDma_Write("ERR,HEADSTEP_FORMAT,DELTA_DEG\r\n");
    return;
  }
  if ((delta_deg < -170) || (delta_deg > 170) || (delta_deg == 0))
  {
    SerialDma_Write("ERR,HEADSTEP_RANGE,-170,170,NONZERO\r\n");
    return;
  }
  if (Vehicle_StepHeading((int16_t)delta_deg) == 0U)
  {
    SerialDma_Write("ERR,HEADSTEP_REJECTED\r\n");
    return;
  }

  (void)snprintf(message, sizeof(message), "ACK,HEADSTEP,%d\r\n", delta_deg);
  SerialDma_Write(message);
}

static void Command_HandleHeadingHold(void)
{
  if (Vehicle_HoldHeading() == 0U)
  {
    SerialDma_Write("ERR,HEADHOLD_REJECTED\r\n");
    return;
  }

  SerialDma_Write("ACK,HEADHOLD\r\n");
}

static uint8_t Command_HandleWheelJog(const char *command)
{
  MotorId id;
  int8_t direction;
  char message[32];

  if ((strlen(command) != 2U) || (command[0] < 'A') || (command[0] > 'D'))
  {
    return 0U;
  }
  if ((command[1] != '+') && (command[1] != '-'))
  {
    return 0U;
  }

  id = (MotorId)(command[0] - 'A');
  direction = (command[1] == '+') ? 1 : -1;
  if (Vehicle_Jog(id, direction) == 0U)
  {
    SerialDma_Write("ERR,MOTOR_DISABLED\r\n");
    return 1U;
  }

  (void)snprintf(message, sizeof(message), "ACK,JOG,%c,%c\r\n", command[0], command[1]);
  SerialDma_Write(message);
  return 1U;
}

static void Command_Normalize(const char *source, char output[COMMAND_SIZE])
{
  size_t length;
  size_t index;

  while (isspace((unsigned char)*source))
  {
    ++source;
  }

  length = strlen(source);
  while ((length > 0U) && isspace((unsigned char)source[length - 1U]))
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

  Command_Normalize(line, command);
  if (command[0] == '\0')
  {
    return;
  }

  if ((strcmp(command, "STOP") == 0) || (strcmp(command, "X") == 0))
  {
    Vehicle_Stop();
    SerialDma_Write("ACK,STOP\r\n");
    return;
  }
  if (strcmp(command, "W") == 0)
  {
    Command_SendMoveAck(command, Vehicle_Move(VEHICLE_FORWARD));
    return;
  }
  if (strcmp(command, "S") == 0)
  {
    Command_SendMoveAck(command, Vehicle_Move(VEHICLE_BACKWARD));
    return;
  }
  if (strcmp(command, "A") == 0)
  {
    Command_SendMoveAck(command, Vehicle_Move(VEHICLE_LEFT));
    return;
  }
  if (strcmp(command, "D") == 0)
  {
    Command_SendMoveAck(command, Vehicle_Move(VEHICLE_RIGHT));
    return;
  }
  if (strcmp(command, "Q") == 0)
  {
    Command_SendMoveAck(command, Vehicle_Move(VEHICLE_ROTATE_LEFT));
    return;
  }
  if (strcmp(command, "E") == 0)
  {
    Command_SendMoveAck(command, Vehicle_Move(VEHICLE_ROTATE_RIGHT));
    return;
  }
  if (strncmp(command, "DRV,", 4U) == 0)
  {
    Command_HandleDrive(command);
    return;
  }
  if (strncmp(command, "SPEED,", 6U) == 0)
  {
    Command_HandleSpeed(command);
    return;
  }
  if ((strncmp(command, "PID,", 4U) == 0) ||
      (strncmp(command, "PIDLIM,", 7U) == 0))
  {
    SerialDma_Write("ERR,PID_FIXED,KP0.8,KI0.1,KD0.0\r\n");
    return;
  }
  if ((strcmp(command, "PIDON,0") == 0) || (strcmp(command, "PIDON,1") == 0))
  {
    if (command[6] == '0')
    {
      Vehicle_Stop();
    }
    SpeedControl_SetEnabled((uint8_t)(command[6] - '0'));
    Command_SendPidAck();
    return;
  }
  if (strncmp(command, "YAWPID,", 7U) == 0)
  {
    SerialDma_Write("ERR,YAWPID_FIXED,KP0.15,KI2.5,KFF0.0\r\n");
    return;
  }
  if ((strcmp(command, "YAWON,0") == 0) || (strcmp(command, "YAWON,1") == 0))
  {
    if (command[6] == '0')
    {
      Vehicle_Stop();
    }
    YawRateControl_SetEnabled((uint8_t)(command[6] - '0'));
    Command_SendYawPidAck();
    return;
  }
  if (strcmp(command, "YAWRESET") == 0)
  {
    YawRateControl_Reset();
    SerialDma_Write("ACK,YAWRESET\r\n");
    return;
  }
  if (strncmp(command, "HEADPID,", 8U) == 0)
  {
    SerialDma_Write("ERR,HEADPID_FIXED,KP5.0,KD1.25,MAX80\r\n");
    return;
  }
  if (strncmp(command, "HEADSTEP,", 9U) == 0)
  {
    Command_HandleHeadingStep(command);
    return;
  }
  if (strcmp(command, "HEADHOLD") == 0)
  {
    Command_HandleHeadingHold();
    return;
  }
  if ((strcmp(command, "HEADON,0") == 0) || (strcmp(command, "HEADON,1") == 0))
  {
    HeadingControl_SetEnabled((uint8_t)(command[7] - '0'));
    Command_SendHeadingAck();
    return;
  }
  if (strcmp(command, "HEADRESET") == 0)
  {
    HeadingControl_Reset();
    SerialDma_Write("ACK,HEADRESET\r\n");
    return;
  }
  if (strcmp(command, "YAWZERO") == 0)
  {
    Gyro_ZeroYaw();
    HeadingControl_Reset();
    SerialDma_Write("ACK,YAWZERO\r\n");
    return;
  }
  if (strcmp(command, "GYROCAL") == 0)
  {
    Vehicle_Stop();
    if (Gyro_StartCalibration() == 0U)
    {
      SerialDma_Write("ERR,MPU6050_OFFLINE\r\n");
      return;
    }
    SerialDma_Write("ACK,GYROCAL\r\n");
    return;
  }
  if (strcmp(command, "PIDRESET") == 0)
  {
    SpeedControl_Reset();
    SerialDma_Write("ACK,PIDRESET\r\n");
    return;
  }
  if (strcmp(command, "ZERO") == 0)
  {
    Encoder_ResetAll();
    SerialDma_Write("ACK,ZERO\r\n");
    Telemetry_SendNow();
    return;
  }
  if (strcmp(command, "STATUS") == 0)
  {
    Telemetry_SendNow();
    return;
  }
  if (strcmp(command, "PING") == 0)
  {
    SerialDma_Write("PONG\r\n");
    return;
  }
  if ((strcmp(command, "HELP") == 0) || (strcmp(command, "?") == 0))
  {
    SerialDma_Write("CMD,W,A,S,D,Q,E,DRV,f,l,y,STOP,SPEED,n,HEADON,0/1,HEADSTEP,deg,HEADHOLD,HEADRESET,GYROCAL,STATUS\r\n");
    return;
  }
  if (Command_HandleWheelJog(command) != 0U)
  {
    return;
  }

  SerialDma_Write("ERR,UNKNOWN_COMMAND\r\n");
}
