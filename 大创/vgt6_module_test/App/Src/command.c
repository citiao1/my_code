#include "command.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "attitude_control.h"

#include "battery.h"
#include "chassis_test.h"
#include "encoder.h"
#include "gyro.h"
#include "ps2.h"
#include "ps2_control.h"
#include "serial_dma.h"
#include "speed_control.h"
#include "stack_usage.h"
#include "telemetry.h"
#include "stm32f4xx_hal.h"

#define COMMAND_SIZE 64U

static uint8_t Command_Hlog(const char *command)
{
  unsigned long offset;
  unsigned long count;

  if (strcmp(command, "HLOG,CLEAR") == 0)
  {
    if (AttitudeControl_GetSnapshot().mode != 0U)
    {
      SerialDma_Write("ERR,HLOG,MOTION_ACTIVE\r\n");
    }
    else
    {
      Telemetry_Hlog_Clear();
      SerialDma_Write("HLOG,CLEAR,OK\r\n");
    }
    return 1U;
  }
  if (strcmp(command, "HLOG,START") == 0)
  {
    if (AttitudeControl_GetSnapshot().mode != 0U)
    {
      SerialDma_Write("ERR,HLOG,MOTION_ACTIVE\r\n");
    }
    else if (Telemetry_Hlog_Start() == 0U)
    {
      SerialDma_Write("ERR,HLOG,ALREADY_ACTIVE\r\n");
    }
    else
    {
      SerialDma_Write("HLOG,START,OK\r\n");
    }
    return 1U;
  }
  if (strcmp(command, "HLOG,STOP") == 0)
  {
    Telemetry_Hlog_Stop();
    SerialDma_Write("HLOG,STOP,OK\r\n");
    return 1U;
  }
  if (strcmp(command, "HLOG,STATUS") == 0)
  {
    (void)Telemetry_Hlog_SendStatus();
    return 1U;
  }
  if (strncmp(command, "HLOG,READ,", 10U) == 0)
  {
    if (AttitudeControl_GetSnapshot().mode != 0U)
    {
      SerialDma_Write("ERR,HLOG,MOTION_ACTIVE\r\n");
    }
    else if (Telemetry_Hlog_IsActive())
    {
      SerialDma_Write("ERR,HLOG,STOP_RECORDING_FIRST\r\n");
    }
    else if ((sscanf(command + 10U, "%lu,%lu", &offset, &count) != 2) ||
             (count > TELEMETRY_HLOG_READ_MAX))
    {
      SerialDma_Write("ERR,HLOG,USE_READ,OFFSET,COUNT_0_8\r\n");
    }
    else
    {
      (void)Telemetry_Hlog_SendRead((uint32_t)offset, (uint32_t)count);
    }
    return 1U;
  }
  return 0U;
}

static uint8_t Command_Attitude(const char *command)
{
  AttitudeControlSnapshot a;
  float p, i, target;
  int forward, left;
  char message[128];
  if (!strcmp(command, "CAPS"))
  {
    SerialDma_Write("CAPS,ATTITUDE_R17,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE\r\n");
    return 1U;
  }
  if (Command_Hlog(command)) return 1U;
  if (!strcmp(command, "YAWGET") || !strcmp(command, "HEADGET"))
  {
    a = AttitudeControl_GetSnapshot();
    if (command[0] == 'Y')
      (void)snprintf(message, sizeof(message), "YAWPID,%.4f,%.4f,60,80,20\r\n",
                     a.rate_kp, a.rate_ki);
    else
      (void)snprintf(message, sizeof(message), "HEADPID,%.4f,%.4f,80,20,D_MEAS\r\n",
                     a.heading_kp, a.heading_kd);
    SerialDma_Write(message);
    return 1U;
  }
  if (!strncmp(command, "YAWPID,", 7U) || !strncmp(command, "HEADPID,", 8U))
  {
    uint8_t rate = command[0] == 'Y';
    if (sscanf(command + (rate ? 7 : 8), "%f,%f", &p, &i) != 2 ||
        !(rate ? AttitudeControl_SetRatePid(p, i) :
                 AttitudeControl_SetHeadingPid(p, i)))
      SerialDma_Write("ERR,ANGLEPID,RANGE_0_100\r\n");
    else SerialDma_Write(rate ? "ACK,YAWPID\r\n" : "ACK,HEADPID\r\n");
    return 1U;
  }
  if (!strcmp(command, "KEEP"))
  {
    SerialDma_Write(AttitudeControl_KeepAlive() ? "ACK,KEEP\r\n" :
                                                  "ERR,KEEP,STOPPED\r\n");
    return 1U;
  }
  if (!strcmp(command, "ANGLESTATUS"))
  {
    Telemetry_SendAttitude();
    return 1U;
  }
  if (!strcmp(command, "ATEL,1") || !strcmp(command, "ATEL,0"))
  {
    Telemetry_SetAttitude(command[5] == '1');
    SerialDma_Write(command[5] == '1' ? "ACK,ATEL,1\r\n" : "ACK,ATEL,0\r\n");
    return 1U;
  }
  if (!strncmp(command, "YAW,", 4U) || !strncmp(command, "HEAD,", 5U))
  {
    uint8_t rate = command[0] == 'Y';
    if (Ps2Control_IsActive())
    {
      SerialDma_Write("ERR,ANGLE,PS2_ACTIVE\r\n");
      return 1U;
    }
    if (sscanf(command + (rate ? 4 : 5), "%f", &target) != 1 ||
        !(rate ? AttitudeControl_SetRate(target, 0, 0) :
                 AttitudeControl_SetHeading(target)))
      SerialDma_Write("ERR,ANGLE,ARM_IMU_OR_RANGE\r\n");
    else
    {
      Telemetry_SetAttitude(1U);
      SerialDma_Write(rate ? "ACK,YAW\r\n" : "ACK,HEAD\r\n");
    }
    return 1U;
  }
  if (!strncmp(command, "HEADMOVE,", 9U))
  {
    if (Ps2Control_IsActive())
    {
      SerialDma_Write("ERR,HEADMOVE,PS2_ACTIVE\r\n");
      return 1U;
    }
    if (sscanf(command + 9, "%f,%d,%d", &target, &forward, &left) != 3 ||
        forward < -200 || forward > 200 || left < -200 || left > 200 ||
        !AttitudeControl_SetHeadingWithDrive(target, (int16_t)forward,
                                             (int16_t)left))
      SerialDma_Write("ERR,HEADMOVE,ARM_IMU_OR_RANGE\r\n");
    else
    {
      Telemetry_SetAttitude(1U);
      SerialDma_Write("ACK,HEADMOVE\r\n");
    }
    return 1U;
  }
  if (!strncmp(command, "TRACKMOVE,", 10U))
  {
    float field_forward, field_left;
    int end = 0;
    if (Ps2Control_IsActive())
    {
      SerialDma_Write("ERR,TRACKMOVE,PS2_ACTIVE\r\n");
      return 1U;
    }
    if (sscanf(command + 10, "%f,%f,%f%n", &target,
               &field_forward, &field_left, &end) != 3 ||
        command[10 + end] != '\0' ||
        !AttitudeControl_SetTrackedDrive(target, field_forward, field_left))
      SerialDma_Write("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
    else
      SerialDma_Write("ACK,TRACKMOVE\r\n");
    /* Query/reply only: this command never enables unsolicited streaming. */
    return 1U;
  }
  if (!strncmp(command, "MOVE,", 5U))
  {
    if (Ps2Control_IsActive())
    {
      SerialDma_Write("ERR,MOVE,PS2_ACTIVE\r\n");
      return 1U;
    }
    if (sscanf(command + 5, "%d,%d,%f", &forward, &left, &target) != 3 ||
        forward < -200 || forward > 200 || left < -200 || left > 200 ||
        !AttitudeControl_SetRate(target, (int16_t)forward, (int16_t)left))
      SerialDma_Write("ERR,MOVE,ARM_IMU_OR_RANGE\r\n");
    else
    {
      Telemetry_SetAttitude(1U);
      SerialDma_Write("ACK,MOVE\r\n");
    }
    return 1U;
  }
  return 0U;
}

static void Command_SendUnknown(const char *raw, const char *normalized)
{
  static const char digits[] = "0123456789ABCDEF";
  static char message[144];
  size_t index;
  size_t count = strlen(raw);

  (void)snprintf(message, sizeof(message),
                 "ERR,UNKNOWN_COMMAND,%s\r\n", normalized);
  SerialDma_Write(message);
  if (count >= COMMAND_SIZE) count = COMMAND_SIZE - 1U;
  memcpy(message, "RXHEX,", 6U);
  for (index = 0U; index < count; ++index)
  {
    uint8_t byte = (uint8_t)raw[index];
    message[6U + index * 2U] = digits[byte >> 4U];
    message[7U + index * 2U] = digits[byte & 0x0FU];
  }
  memcpy(&message[6U + count * 2U], "\r\n", 3U);
  SerialDma_Write(message);
}

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
  if (strcmp(command, "STACK") == 0)
  {
    StackUsageSnapshot stack = StackUsage_GetSnapshot();
    char message[80];
    (void)snprintf(message, sizeof(message), "STACK,%lu,%lu,%lu,%u\r\n",
                   (unsigned long)stack.allocated_bytes,
                   (unsigned long)stack.used_bytes,
                   (unsigned long)stack.free_bytes, stack.guard_ok);
    SerialDma_Write(message);
    return;
  }
  if (strcmp(command, "UART") == 0)
  {
    SerialUartSnapshot uart = SerialDma_GetUartSnapshot();
    char message[160];
    (void)snprintf(message, sizeof(message),
                   "UART,%lu,%lu,%lu,0x%02lX,%lu,%lu,%lu,%lu,%lu\r\n",
                   (unsigned long)uart.baud, (unsigned long)uart.pclk_hz,
                   (unsigned long)uart.brr, (unsigned long)uart.last_error,
                   (unsigned long)uart.parity_errors,
                   (unsigned long)uart.noise_errors,
                   (unsigned long)uart.framing_errors,
                   (unsigned long)uart.overrun_errors,
                   (unsigned long)uart.dma_errors);
    SerialDma_Write(message);
    return;
  }
  if ((strcmp(command, "VER") == 0) ||
      (strcmp(command, "VERSION") == 0))
  {
    SerialDma_Write("VER,VGT6_BM50_ATTITUDE_R17\r\n");
    return;
  }
  if (strcmp(command, "PS2STATUS") == 0)
  {
    Ps2Snapshot ps2 = Ps2_GetSnapshot();
    Ps2ControlSnapshot control = Ps2Control_GetSnapshot();
    char message[160];
    (void)snprintf(message, sizeof(message),
                   "PS2,%u,%u,0x%02X,0x%04X,%u,%u,%u,%u,%u,%u,%u\r\n",
                   ps2.connected, control.enabled, ps2.mode, ps2.buttons,
                   ps2.left_x, ps2.left_y, ps2.right_x, ps2.right_y,
                   control.speed_rpm, ps2.read_failures,
                   (unsigned int)(HAL_GetTick() - ps2.last_tick));
    SerialDma_Write(message);
    return;
  }
  if (strcmp(command, "PS2RAW") == 0)
  {
    Ps2Snapshot ps2 = Ps2_GetSnapshot();
    char message[128];
    (void)snprintf(message, sizeof(message),
                   "PS2RAW,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%u,%u\r\n",
                   ps2.raw_frame[0], ps2.raw_frame[1], ps2.raw_frame[2],
                   ps2.raw_frame[3], ps2.raw_frame[4], ps2.raw_frame[5],
                   ps2.raw_frame[6], ps2.raw_frame[7], ps2.raw_frame[8],
                   ps2.valid, ps2.config_attempts);
    SerialDma_Write(message);
    return;
  }
  if (strcmp(command, "LINK") == 0)
  {
    SerialDmaSnapshot link = SerialDma_GetSnapshot();
    char message[128];
    (void)snprintf(message, sizeof(message),
                   "LINK,%lu,%lu,%lu,%lu,%lu,%u\r\n",
                   (unsigned long)link.rx_bytes,
                   (unsigned long)link.rx_lines,
                   (unsigned long)link.rx_overflows,
                   (unsigned long)link.tx_dropped,
                   (unsigned long)link.uart_errors,
                   Telemetry_IsPeriodic());
    SerialDma_Write(message);
    return;
  }
  if (StackUsage_IsHealthy() == 0U)
  {
    ChassisTest_Disarm();
    Telemetry_SetPeriodic(0U);
    SerialDma_Write("ERR,STACK_GUARD,MOTORS_DISABLED\r\n");
    return;
  }
  if (Command_Attitude(command)) return;
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
    Ps2Control_Stop();
    ChassisTest_Disarm();
    SerialDma_Write("ACK,DISARM\r\n");
    return;
  }
  if (strcmp(command, "STOP") == 0)
  {
    Ps2Control_Stop();
    ChassisTest_Stop();
    /*
     * STOP brakes and disables the control loops, but it does not change
     * the user's telemetry selection. TEL,0 remains the explicit way to
     * stop a periodic stream.
     */
    SerialDma_Write("ACK,STOP\r\n");
    return;
  }
  if (strcmp(command, "SPEED,1") == 0)
  {
    Ps2Control_Stop();
    ChassisTest_Stop();
    if (SpeedControl_Enable(1U) == 0U)
    {
      SerialDma_Write("ERR,SPEED,ARM_FIRST\r\n");
    }
    else
    {
      Telemetry_SetPeriodic(1U);
      SerialDma_Write("ACK,SPEED,1\r\n");
    }
    return;
  }
  if (strcmp(command, "SPEED,0") == 0)
  {
    Ps2Control_Stop();
    ChassisTest_Stop();
    SerialDma_Write("ACK,SPEED,0\r\n");
    return;
  }
  if (strncmp(command, "SPEED,", 6U) == 0)
  {
    unsigned long limit_rpm;
    if ((sscanf(command, "SPEED,%lu", &limit_rpm) != 1) ||
        (limit_rpm == 0UL) || (limit_rpm > SPEED_CONTROL_MAX_TARGET_RPM) ||
        (SpeedControl_SetTargetLimit((uint16_t)limit_rpm) == 0U))
    {
      SerialDma_Write("ERR,SPEED,RANGE_1_200\r\n");
    }
    else
    {
      SerialDma_Write("ACK,SPEED,LIMIT\r\n");
    }
    return;
  }
  if (strncmp(command, "SPD,", 4U) == 0)
  {
    char channel_name;
    int speed_rpm;
    MotorChannel channel;

    if (Ps2Control_IsActive())
    {
      SerialDma_Write("ERR,SPD,PS2_ACTIVE\r\n");
      return;
    }
    if (AttitudeControl_GetSnapshot().mode) ChassisTest_Stop();
    if ((sscanf(command, "SPD,%c,%d", &channel_name, &speed_rpm) != 2) ||
        (Command_ParseMotor(channel_name, &channel) == 0U) ||
        (SpeedControl_SetTarget(channel, speed_rpm) == 0U))
    {
      SerialDma_Write("ERR,SPD,USE_SPD,A,RPM\r\n");
    }
    else
    {
      SerialDma_Write("ACK,SPD\r\n");
    }
    return;
  }
  if (strncmp(command, "DRV,", 4U) == 0)
  {
    int forward_rpm;
    int left_rpm;
    int rotate_rpm;

    if (Ps2Control_IsActive())
    {
      SerialDma_Write("ERR,DRV,PS2_ACTIVE\r\n");
      return;
    }
    if (AttitudeControl_GetSnapshot().mode) ChassisTest_Stop();
    if ((sscanf(command, "DRV,%d,%d,%d", &forward_rpm, &left_rpm,
                &rotate_rpm) != 3) ||
        forward_rpm < -200 || forward_rpm > 200 ||
        left_rpm < -200 || left_rpm > 200 ||
        rotate_rpm < -200 || rotate_rpm > 200 ||
        (SpeedControl_SetDrive((int16_t)forward_rpm, (int16_t)left_rpm,
                               (int16_t)rotate_rpm) == 0U))
    {
      SerialDma_Write("ERR,DRV,SPEED_DISABLED_OR_RANGE\r\n");
    }
    else
    {
      SerialDma_Write("ACK,DRV\r\n");
    }
    return;
  }
  if (strncmp(command, "PID,", 4U) == 0)
  {
    char channel_name;
    float kp;
    float ki;
    float kd;
    MotorChannel channel;

    if ((sscanf(command, "PID,%c,%f,%f,%f", &channel_name, &kp, &ki, &kd) != 4) ||
        (Command_ParseMotor(channel_name, &channel) == 0U))
    {
      SerialDma_Write("ERR,PID,USE_PID,A,KP,KI,KD\r\n");
    }
    else if (SpeedControl_SetPid(channel, kp, ki, kd) == 0U)
    {
      SerialDma_Write("ERR,PID,SPEED_TUNABLE_R17_RANGE\r\n");
    }
    else
    {
      SerialDma_Write("ACK,PID\r\n");
    }
    return;
  }
  if (strncmp(command, "SPDLIMIT,", 9U) == 0)
  {
    unsigned long limit_rpm;
    if ((sscanf(command, "SPDLIMIT,%lu", &limit_rpm) != 1) ||
        (limit_rpm == 0UL) || (limit_rpm > SPEED_CONTROL_MAX_TARGET_RPM) ||
        (SpeedControl_SetTargetLimit((uint16_t)limit_rpm) == 0U))
    {
      SerialDma_Write("ERR,SPDLIMIT,RANGE_1_200\r\n");
    }
    else
    {
      SerialDma_Write("ACK,SPDLIMIT\r\n");
    }
    return;
  }
  if (strcmp(command, "SPEEDLIMIT") == 0)
  {
    SpeedControlSnapshot speed = SpeedControl_GetSnapshot();
    char message[40];
    (void)snprintf(message, sizeof(message), "SPEEDLIMIT,%u\r\n",
                   speed.target_limit_rpm);
    SerialDma_Write(message);
    return;
  }
  if (strcmp(command, "PIDGET") == 0)
  {
    SpeedControlSnapshot speed = SpeedControl_GetSnapshot();
    char message[192];
    (void)snprintf(message, sizeof(message),
                   "PID,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f\r\n",
                   speed.kp[0], speed.ki[0], speed.kd[0],
                   speed.kp[1], speed.ki[1], speed.kd[1],
                   speed.kp[2], speed.ki[2], speed.kd[2],
                   speed.kp[3], speed.ki[3], speed.kd[3]);
    SerialDma_Write(message);
    return;
  }
  if (strcmp(command, "SPEEDSTATUS") == 0)
  {
    Telemetry_SendSpeedStatus();
    return;
  }
  if (strcmp(command, "TEL,1") == 0)
  {
    Telemetry_SetPeriodic(1U);
    SerialDma_Write("ACK,TEL,1\r\n");
    return;
  }
  if (strcmp(command, "TEL,0") == 0)
  {
    Telemetry_SetPeriodic(0U);
    SerialDma_Write("ACK,TEL,0\r\n");
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
    ChassisTest_Stop();
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
    ChassisTest_Stop();
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
    SerialDma_Write("CMD,VER,LINK,UART,STACK,STATUS,ARM,DISARM,STOP,PING\r\n");
    SerialDma_Write("CMD,ARM,DISARM,STOP,A+,A-,B+,B-,C+,C-,D+,D-\r\n");
    SerialDma_Write("CMD,JOG,A,+/-1..30,50..5000,ENC,ENCZERO,CPR,380\r\n");
    SerialDma_Write("CMD,BAT,RECOVER,A,STATUS,CAL,ZERO,WHO,PING,HELP\r\n");
    SerialDma_Write("CMD,SPEED,0/1,SPD,A,RPM,DRV,F,L,R,PID,A,KP,KI,KD,PIDGET,SPEEDSTATUS,SPEEDLIMIT,TEL,0/1\r\n");
    SerialDma_Write("CMD,CAPS,PS2STATUS,YAW,DPS,HEAD,DEG,HEADMOVE,DEG,F,L,TRACKMOVE,DEG,FX,FY,MOVE,F,L,DPS,KEEP,YAWPID,KP,KI,HEADPID,KP,KD,YAWGET,HEADGET,ANGLESTATUS,ATEL,0/1\r\n");
    SerialDma_Write("CMD,HLOG,CLEAR,START,STOP,STATUS,READ,OFFSET,COUNT_0_8\r\n");
    return;
  }

  Command_SendUnknown(line, command);
}
