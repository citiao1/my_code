/* Host regression: real transport, commands, telemetry and speed-loop code.
 * Only HAL and board I/O are stubbed. No board/probe is accessed. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "serial_dma.h"
#include "command.h"
#include "chassis_test.h"
#include "speed_control.h"
#include "telemetry.h"
#include "gyro.h"
#include "encoder.h"
#include "battery.h"
#include "stack_usage.h"
#include "attitude_control.h"
#include "mecanum.h"
#include "ps2.h"
#include "ps2_control.h"
#include "tof.h"

static DMA_HandleTypeDef mock_dma;
static USART_TypeDef mock_usart = {364U};
UART_HandleTypeDef huart2 = { &mock_dma, {115200U}, &mock_usart, 0U };
static uint8_t *rx_buffer;
static uint16_t rx_capacity;
static uint32_t tick;
static char transmitted[8192];
static size_t tx_length;
static unsigned cases;
static EncoderSnapshot encoder;
static uint8_t stack_healthy = 1U;
static GyroSnapshot gyro_mock;
static Ps2Snapshot ps2_mock;
static Ps2ControlSnapshot ps2_control_mock;
static TofSnapshot tof_mock;
static int motor_applied[4];
static uint8_t tx_autocomplete = 1U;
TofSnapshot Tof_GetSnapshot(void) { return tof_mock; }

uint32_t HAL_GetTick(void) { return tick; }
uint32_t HAL_RCC_GetPCLK1Freq(void) { return 42000000U; }
uint8_t StackUsage_IsHealthy(void) { return stack_healthy; }
StackUsageSnapshot StackUsage_GetSnapshot(void)
{ StackUsageSnapshot value = {4096U, 1200U, 2896U, stack_healthy}; return value; }
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *uart,
                                              uint8_t *data, uint16_t size)
{
  assert(uart == &huart2);
  rx_buffer = data;
  rx_capacity = size;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *uart,
                                       uint8_t *data, uint16_t size)
{
  assert(tx_length + size < sizeof(transmitted));
  memcpy(transmitted + tx_length, data, size);
  tx_length += size;
  transmitted[tx_length] = '\0';
  /* Deliberately preempt StartTx before it returns to test busy ownership. */
  if (tx_autocomplete) HAL_UART_TxCpltCallback(uart);
  return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *uart)
{ (void)uart; return HAL_OK; }
uint8_t Motor_Init(void) { return 1U; }
void Motor_Stop(MotorChannel channel) { motor_applied[channel] = 0; }
void Motor_StopAll(void) { memset(motor_applied, 0, sizeof(motor_applied)); }
uint8_t Motor_Run(MotorChannel channel, int8_t direction, uint8_t duty)
{ motor_applied[channel] = direction * duty; return 1U; }
uint8_t Motor_Recover(MotorChannel channel) { (void)channel; return 1U; }
void Encoder_Zero(void) { memset(&encoder, 0, sizeof(encoder)); }
uint8_t Encoder_SetCountsPerRevolution(uint16_t cpr) { return cpr >= 4U; }
EncoderSnapshot Encoder_GetSnapshot(void) { return encoder; }
BatterySnapshot Battery_GetSnapshot(void)
{ BatterySnapshot value = { 1000U, 12000U, 0U, 1U }; return value; }
GyroSnapshot Gyro_GetSnapshot(void)
{ return gyro_mock; }
uint8_t Gyro_StartCalibration(void) { return 1U; }
void Gyro_ZeroYaw(void) {}
Ps2Snapshot Ps2_GetSnapshot(void) { return ps2_mock; }
uint8_t Ps2_IsPressed(uint16_t button)
{ return (ps2_mock.buttons & button) != 0U; }
void Ps2Control_Stop(void)
{
  ps2_control_mock.enabled = 0U;
  ps2_control_mock.forward_rpm = 0;
  ps2_control_mock.left_rpm = 0;
  ps2_control_mock.rotate_rpm = 0;
}
uint8_t Ps2Control_IsActive(void) { return ps2_control_mock.enabled; }
Ps2ControlSnapshot Ps2Control_GetSnapshot(void) { return ps2_control_mock; }

static void drain(void)
{
  unsigned i;
  for (i = 0U; i < 32U; ++i) SerialDma_Process();
}
static void clear_tx(void)
{ tx_length = 0U; transmitted[0] = '\0'; }
static void init(void)
{
  tx_autocomplete = 1U;
  tick = 100U;
  clear_tx();
  memset(&encoder, 0, sizeof(encoder));
  memset(&tof_mock, 0, sizeof(tof_mock));
  stack_healthy = 1U;
  huart2.ErrorCode = 0U;
  assert(ChassisTest_Init() == 1U);
  SpeedControl_Init();
  AttitudeControl_Init();
  memset(&gyro_mock, 0, sizeof(gyro_mock));
  gyro_mock.ready = 1U;
  memset(&ps2_mock, 0, sizeof(ps2_mock));
  memset(&ps2_control_mock, 0, sizeof(ps2_control_mock));
  Telemetry_Init();
  assert(SerialDma_Init(Command_HandleLine) == 1U);
}
static void burst(const char *data, size_t size, int process)
{
  assert(size <= rx_capacity);
  memcpy(rx_buffer, data, size);
  HAL_UARTEx_RxEventCallback(&huart2, (uint16_t)size);
  if (process) drain();
}
static void line(const char *command)
{
  clear_tx();
  burst(command, strlen(command), 1);
}
static void expect(const char *reply)
{
  if (strcmp(transmitted, reply) != 0)
  {
    fprintf(stderr, "expected [%s], got [%s]\n", reply, transmitted);
    assert(0);
  }
  ++cases;
}
static void test_every_split(void)
{
  static const char *commands[] = {
    "VER\n", "ARM\n", "SPDLIMIT,120\n", "SPEED,1\n",
    "PID,A,1.2,0.0991,0\n", "SPD,A,60\n", "TEL,1\n",
    "SPEED,0\n", "SPEED,120\n", "STOP\n", "PING\r\n"
  };
  static const char *replies[] = {
    "VER,VGT6_BM50_ATTITUDE_R24\r\n", "ACK,ARM,WHEELS_OFF_GROUND\r\n",
    "ACK,SPDLIMIT\r\n", "ACK,SPEED,1\r\n", "ACK,PID\r\n",
    "ACK,SPD\r\n", "ACK,TEL,1\r\n", "ACK,SPEED,0\r\n",
    "ACK,SPEED,LIMIT\r\n", "ACK,STOP\r\n", "PONG\r\n"
  };
  size_t c, split, length;
  for (c = 0U; c < sizeof(commands) / sizeof(commands[0]); ++c)
  {
    length = strlen(commands[c]);
    for (split = 1U; split < length; ++split)
    {
      init();
      ChassisTest_Arm();
      burst(commands[c], split, 1);
      /* CR itself is an explicit terminator, unlike IDLE. */
      if (commands[c][split - 1U] != '\r') expect("");
      burst(commands[c] + split, length - split, 1);
      expect(replies[c]);
    }
  }
  init();
  ChassisTest_Arm();
  for (split = 0U; split < strlen(commands[4]); ++split)
    burst(commands[4] + split, 1U, 1);
  expect("ACK,PID\r\n");
}
static void test_stop_restart_and_tel(void)
{
  init();
  line("SPEED,1\n"); expect("ERR,SPEED,ARM_FIRST\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  line("SPD,A,60\n"); expect("ACK,SPD\r\n");
  encoder.rpm_x10[0] = 500;
  SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().actual_rpm_x10[0] == 150);
  line("STOP\n"); expect("ACK,STOP\r\n");
  assert(SpeedControl_GetSnapshot().enabled == 0U);
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 0);
  assert(Telemetry_IsPeriodic() == 1U);
  assert(ChassisTest_GetSnapshot().armed == 1U);
  clear_tx();
  tick += 4000U; Telemetry_Process(); drain();
  assert(strncmp(transmitted, "TEL,", 4U) == 0);
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  line("SPD,A,60\n"); expect("ACK,SPD\r\n");
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 60);
  line("SPEED,120\n"); expect("ACK,SPEED,LIMIT\r\n");
  assert(SpeedControl_GetSnapshot().enabled == 1U);
  line("SPDLIMIT,1\n"); expect("ACK,SPDLIMIT\r\n");
  assert(SpeedControl_GetSnapshot().target_limit_rpm == 1U);
  line("SPEEDLIMIT\n"); expect("SPEEDLIMIT,1\r\n");
  line("TEL,0\n"); expect("ACK,TEL,0\r\n");
  clear_tx(); tick += 4000U; Telemetry_Process(); drain(); expect("");
  line("TEL,1\n"); expect("ACK,TEL,1\r\n");
  clear_tx(); tick += 299U; Telemetry_Process(); drain(); expect("");
  tick += 1U; Telemetry_Process(); drain();
  assert(strncmp(transmitted, "TEL,", 4U) == 0);
  assert(strstr(transmitted, "\r\n") != NULL);
  { unsigned commas = 0U; const char *p = transmitted;
    while (*p != '\0') { if (*p++ == ',') ++commas; }
    assert(commas == 26U); ++cases; }
}
static void test_bad_lines_and_recovery(void)
{
  char long_line[64];
  char full_burst[64];
  init();
  line("NOTREAL\n"); expect("ERR,UNKNOWN_COMMAND,NOTREAL\r\nRXHEX,4E4F545245414C\r\n");
  clear_tx();
  memset(long_line, 'X', sizeof(long_line));
  burst(long_line, sizeof(long_line), 1);
  burst("\nPING\n", 6U, 1);
  expect("ERR,LINE_TOO_LONG\r\nPONG\r\n");
  clear_tx();
  burst("PI\x80NG\nPING\n", 11U, 1);
  expect("ERR,NON_ASCII\r\nPONG\r\n");
  clear_tx();
  memset(full_burst, 'X', sizeof(full_burst));
  for (unsigned i = 0U; i < 5U; ++i) burst(full_burst, sizeof(full_burst), 0);
  drain(); burst("\nPING\n", 6U, 1);
  expect("ERR,RX_OVERFLOW\r\nPONG\r\n");
  assert(SerialDma_GetSnapshot().rx_overflows > 0U);
  HAL_UART_ErrorCallback(&huart2); drain();
  line("\nPING\n"); expect("PONG\r\n");
  assert(SerialDma_GetSnapshot().uart_errors == 1U);
}
static void test_uart_fault_drops_partial_targets(void)
{
  init();
  ChassisTest_Arm();
  burst("SPD,A,", 6U, 1);
  huart2.ErrorCode = HAL_UART_ERROR_FE | HAL_UART_ERROR_ORE;
  HAL_UART_ErrorCallback(&huart2);
  drain();
  expect("ERR,UART_RX,QUERY_UART\r\n");
  clear_tx();
  burst("30\n", 3U, 1);
  expect("");
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 0);
  line("\nSPD,A,30\n"); expect("ACK,SPD\r\n");
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 30);
  line("\nUART\n");
  expect("UART,115200,42000000,364,0x0C,0,0,1,1,0\r\n");
}
static void test_stack_guard_and_diagnostics(void)
{
  init();
  line("STACK\n"); expect("STACK,4096,1200,2896,1\r\n");
  line("UART\n"); expect("UART,115200,42000000,364,0x00,0,0,0,0,0\r\n");
  line("STATUS\n");
  assert(strstr(transmitted, "IMU,") != NULL);
  assert(strstr(transmitted, "TEL,") != NULL);
  ++cases;
  stack_healthy = 0U;
  line("ARM\n"); expect("ERR,STACK_GUARD,MOTORS_DISABLED\r\n");
  assert(ChassisTest_GetSnapshot().armed == 0U);
  line("SPD,A,30\n"); expect("ERR,STACK_GUARD,MOTORS_DISABLED\r\n");
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 0);
  line("STATUS\n"); expect("ERR,STACK_GUARD,MOTORS_DISABLED\r\n");
  line("STACK\n"); expect("STACK,4096,1200,2896,0\r\n");
}

static void test_tunable_gains_and_target_ceiling(void)
{
  unsigned index;
  SpeedControlSnapshot speed;
  const float ki[4] = {0.0991f,0.0998f,0.0998f,0.1008f};
  init();
  speed = SpeedControl_GetSnapshot();
  assert(speed.target_limit_rpm == 160U);
  for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
  {
    assert(speed.kp[index] == 1.2f);
    assert(speed.ki[index] == ki[index]);
    assert(speed.kd[index] == 0.0f);
    assert(SpeedControl_SetPid((MotorChannel)index, 0.8f, 0.1f, 0.0f) == 1U);
    speed = SpeedControl_GetSnapshot();
    assert(speed.kp[index] == 0.8f);
    assert(speed.ki[index] == 0.1f);
    assert(speed.kd[index] == 0.0f);
    assert(SpeedControl_SetPid((MotorChannel)index, 1.2f, ki[index], 0.0f) == 1U);
    ++cases;
  }
  assert(SpeedControl_SetPid(MOTOR_CHANNEL_A, NAN, 0.2f, 0.0f) == 0U);
  assert(SpeedControl_SetPid(MOTOR_CHANNEL_A, 1.2f, INFINITY, 0.0f) == 0U);
  line("PIDGET\n");
  expect("PID,1.2000,0.0991,0.0000,1.2000,0.0998,0.0000,1.2000,0.0998,0.0000,1.2000,0.1008,0.0000\r\n");
  line("PID,A,0.8,0.1,0\n"); expect("ACK,PID\r\n");
  line("PIDGET\n");
  assert(strstr(transmitted, "PID,0.8000,0.1000,0.0000") != NULL);
  line("PID,B,1.2,0.2,1\n"); expect("ACK,PID\r\n");
  line("PID,A,NAN,0.2,0\n"); expect("ERR,PID,SPEED_TUNABLE_R24_RANGE\r\n");
  line("PID,A,1.2,INF,0\n"); expect("ERR,PID,SPEED_TUNABLE_R24_RANGE\r\n");
  line("SPDLIMIT,201\n"); expect("ERR,SPDLIMIT,RANGE_1_200\r\n");
  line("SPDLIMIT,65736\n"); expect("ERR,SPDLIMIT,RANGE_1_200\r\n");
  line("SPEED,1000\n"); expect("ERR,SPEED,RANGE_1_200\r\n");
  assert(SpeedControl_GetSnapshot().target_limit_rpm == 160U);
  line("SPD,A,999\n"); expect("ACK,SPD\r\n");
  line("SPD,B,-999\n"); expect("ACK,SPD\r\n");
  speed = SpeedControl_GetSnapshot();
  assert(speed.target_rpm[0] == 160);
  assert(speed.target_rpm[1] == -160);
  line("SPDLIMIT,180\n"); expect("ACK,SPDLIMIT\r\n");
  speed = SpeedControl_GetSnapshot();
  assert(speed.target_rpm[0] == 160);
  assert(speed.target_rpm[1] == -160);
  line("SPDLIMIT,200\n"); expect("ACK,SPDLIMIT\r\n");
  line("SPEEDLIMIT\n"); expect("SPEEDLIMIT,200\r\n");
  line("PID,A,1.2,0.0991,0\n"); expect("ACK,PID\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  line("SPD,A,60\n"); expect("ACK,SPD\r\n");
  encoder.rpm_x10[0] = 500;
  SpeedControl_Process(10U);
  line("PID,A,1.2,0.0991,0\n"); expect("ERR,PID,STOP_AND_STOP_RECORDING_FIRST\r\n");
  SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().actual_rpm_x10[0] == 255);
  /* Matching legacy PID writes must not reset the active feedback filter. */
  line("SPD,A,200\n"); expect("ACK,SPD\r\n");
  line("SPD,B,-200\n"); expect("ACK,SPD\r\n");
  encoder.rpm_x10[0] = 0;
  for (index = 0U; index < 100U; ++index)
  {
    SpeedControl_Process(10U);
    speed = SpeedControl_GetSnapshot();
    assert(abs(speed.output_percent[0]) <= 60);
    assert(abs(speed.output_percent[1]) <= 60);
  }
  assert(speed.output_percent[0] == 60);
  assert(speed.output_percent[1] == -60);
  ++cases;
  line("STOP\n"); expect("ACK,STOP\r\n");
  assert(SpeedControl_GetSnapshot().kp[0] == 1.2f);
  SpeedControl_Init();
  assert(SpeedControl_GetSnapshot().target_limit_rpm == 160U);
  assert(SpeedControl_GetSnapshot().ki[0] == ki[0]);
}
static void test_attitude_cascade_and_stop(void)
{
  AttitudeControlSnapshot a;
  init();
  line("CAPS\n"); expect("CAPS,ATTITUDE_R24,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE,HLOG_US,TOF_DL1B,WLOG_US,TOF_GUARD,FRONT_REVERSED,TOF_FAST,TOF_GUARD_200_100_40,UART_QOS,SERIAL_115200\r\n");
  line("YAW,15\n"); expect("ERR,ANGLE,ARM_IMU_OR_RANGE\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("HEAD,-179\n"); expect("ACK,HEAD\r\n");
  gyro_mock.yaw_deg = 179.0f;
  AttitudeControl_Process(20);
  a=AttitudeControl_GetSnapshot();
  assert(a.error_deg==2.0f && a.output_dps==12.0f);
  gyro_mock.rate_dps=2.0f;
  AttitudeControl_Process(20);
  assert(AttitudeControl_GetSnapshot().output_dps==9.5f);
  line("HEADMOVE,15,20,0\n"); expect("ACK,HEADMOVE\r\n");
  line("YAWPID,0.25,1.2\n"); expect("ERR,ANGLEPID,STOP_AND_STOP_RECORDING_FIRST\r\n");
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("YAWPID,0.25,1.2\n"); expect("ACK,YAWPID\r\n");
  line("YAWGET\n"); expect("YAWPID,0.2500,1.2000,60,80,20\r\n");
  line("HEADPID,3,0.5\n"); expect("ACK,HEADPID\r\n");
  line("HEADGET\n"); expect("HEADPID,3.0000,0.5000,80,20,D_MEAS\r\n");
  line("HEADPID,NAN,1\n"); expect("ERR,ANGLEPID,RANGE_0_100\r\n");
  line("YAW,81\n"); expect("ERR,ANGLE,ARM_IMU_OR_RANGE\r\n");
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("HLOG,STATUS\n"); expect("HLOG,STATUS,0,0,1024,0,0\r\n");
  line("HLOG,START\n"); expect("HLOG,START,OK\r\n");
  line("HEAD,15\n"); expect("ACK,HEAD\r\n");
  AttitudeControl_Process(20);
  line("HLOG,STOP\n"); expect("HLOG,STOP,OK\r\n");
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("HLOG,STATUS\n");
  assert(strstr(transmitted, "HLOG,STATUS,0,1,1024,0,1\r\n") != NULL);
  line("HLOG,READ,0,1\n");
  assert(strstr(transmitted, "HLOG,DATA,0,") != NULL);
  assert(strstr(transmitted, "HLOG,END,0,1,1\r\n") != NULL);
  line("HLOG,CLEAR\n"); expect("HLOG,CLEAR,OK\r\n");
  AttitudeControl_Process(20);
  assert(!SpeedControl_GetSnapshot().enabled && !AttitudeControl_GetSnapshot().mode);
  line("YAW,15\n"); expect("ACK,YAW\r\n");
  AttitudeControl_Process(20);
  assert(SpeedControl_GetSnapshot().enabled);
  line("KEEP\n"); expect("ACK,KEEP\r\n");
  tick += 1501;
  AttitudeControl_Process(20);
  assert(!SpeedControl_GetSnapshot().enabled && !AttitudeControl_GetSnapshot().mode);
  line("YAW,15\n"); expect("ACK,YAW\r\n");
  gyro_mock.read_failures=1;
  AttitudeControl_Process(20);
  assert(!SpeedControl_GetSnapshot().enabled);
  gyro_mock.read_failures=0;
  line("YAW,15\n"); expect("ACK,YAW\r\n");
  clear_tx(); tick+=200; Telemetry_Process(); drain();
  assert(strncmp(transmitted,"ATEL,",5)==0 && tx_length<256);
  line("TEL,1\n"); expect("ACK,TEL,1\r\n");
  clear_tx(); tick+=300; Telemetry_Process(); drain();
  assert(strncmp(transmitted,"TEL,",4)==0);
  line("CAL\n"); expect("ACK,CAL,KEEP_STILL\r\n");
  assert(!AttitudeControl_GetSnapshot().mode && !SpeedControl_GetSnapshot().enabled);
  ++cases;
}

static void test_mecanum_installed_numbering(void)
{
  MecanumWheelTargets targets;

  Mecanum_CalculateTargets(100, 0, 0, 200, &targets);
  assert(targets.motor1_right_front == 100);
  assert(targets.motor2_left_front == 100);
  assert(targets.motor3_left_rear == 100);
  assert(targets.motor4_right_rear == 100);

  Mecanum_CalculateTargets(0, 100, 0, 200, &targets);
  assert(targets.motor1_right_front == 100);
  assert(targets.motor2_left_front == -100);
  assert(targets.motor3_left_rear == 100);
  assert(targets.motor4_right_rear == -100);

  Mecanum_CalculateTargets(0, 0, 100, 200, &targets);
  assert(targets.motor1_right_front == 100);
  assert(targets.motor2_left_front == -100);
  assert(targets.motor3_left_rear == -100);
  assert(targets.motor4_right_rear == 100);
  ++cases;
}

static void test_rate_and_heading_ram_logs(void)
{
  AttitudeControlSnapshot sample;
  init();
  line("HLOG,CLEAR\n"); expect("HLOG,CLEAR,OK\r\n");
  line("HLOG,START\n"); expect("HLOG,START,OK\r\n");
  line("HLOG,READ,0,1\n"); expect("ERR,HLOG,STOP_RECORDING_FIRST\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("YAW,15\n"); expect("ACK,YAW\r\n");
  gyro_mock.rate_dps = 10.0f;
  tick += 20U;
  AttitudeControl_Process(20U);
  assert(Telemetry_Hlog_Count() == 1U);
  line("HLOG,READ,0,1\n"); expect("ERR,HLOG,MOTION_ACTIVE\r\n");
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("HLOG,STOP\n"); expect("HLOG,STOP,OK\r\n");
  line("HLOG,READ,0,1\n");
  expect("HLOG,DATA,0,120,0,0,0,15000,10000,0,1088,0,0,0,1,120000\r\nHLOG,END,0,1,1\r\n");
  /* Direct snapshots isolate RAM capacity/mode serialization from hardware. */
  Telemetry_Hlog_Clear();
  assert(Telemetry_Hlog_Start());
  memset(&sample, 0, sizeof(sample));
  Telemetry_Hlog_Record(&sample, tick, tick * 1000U);
  assert(Telemetry_Hlog_Count() == 0U);
  sample.mode = 2U;
  tick += 20U; Telemetry_Hlog_Record(&sample, tick, tick * 1000U);
  sample.mode = 1U;
  for (unsigned index = 0; index < TELEMETRY_HLOG_CAPACITY; ++index)
  {
    tick += 20U;
    Telemetry_Hlog_Record(&sample, tick, tick * 1000U);
  }
  assert(Telemetry_Hlog_Count() == TELEMETRY_HLOG_CAPACITY);
  line("HLOG,STATUS\n"); expect("HLOG,STATUS,1,1024,1024,1,1025\r\n");
  line("HLOG,STOP\n"); expect("HLOG,STOP,OK\r\n");
  line("HLOG,CLEAR\n"); expect("HLOG,CLEAR,OK\r\n");
}

static void test_tracked_drive_frame_rest_and_stop(void)
{
  AttitudeControlSnapshot a;
  init();
  line("TRACKMOVE,0,30,0\n"); expect("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("TRACKMOVE,0,30,0\n"); expect("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  line("TRACKMOVE,0,NAN,0\n"); expect("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
  line("TRACKMOVE,0,80,80\n"); expect("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
  line("TRACKMOVE,0,30,0,JUNK\n"); expect("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
  line("TRACKMOVE,90,30,0\n"); expect("ACK,TRACKMOVE\r\n");
  gyro_mock.yaw_deg = 90.0f;
  for (int step = 0; step < 20; ++step) AttitudeControl_Process(20U);
  a = AttitudeControl_GetSnapshot();
  assert(a.forward_rpm == 0 && a.left_rpm == -30);
  assert(a.output_rpm == 0.0f && a.target_deg == 90.0f);
  /* A held world vector follows ACTUAL yaw, even if its target is unchanged. */
  gyro_mock.yaw_deg = 0.0f;
  AttitudeControl_Process(20U);
  a = AttitudeControl_GetSnapshot();
  assert(a.forward_rpm == 30 && a.left_rpm == 0);
  line("TRACKMOVE,0,0,0\n"); expect("ACK,TRACKMOVE\r\n");
  for (int step = 0; step < 10; ++step) AttitudeControl_Process(20U);
  for (int step = 0; step < 400; ++step)
  {
    gyro_mock.yaw_deg = step % 2 ? 0.2f : -0.2f;
    gyro_mock.rate_dps = step % 2 ? 0.8f : -0.8f;
    /* Repeated commands/KEEP must not reset the quiet gate. */
    assert(AttitudeControl_SetTrackedDrive(0.0f, 0.0f, 0.0f));
    AttitudeControl_Process(20U);
    SpeedControl_Process(10U);
    a = AttitudeControl_GetSnapshot();
    assert(a.output_rpm == 0.0f && a.target_deg == 0.0f);
    for (int wheel = 0; wheel < 4; ++wheel)
      assert(SpeedControl_GetSnapshot().output_percent[wheel] == 0);
  }
  gyro_mock.yaw_deg = 1.0f;
  AttitudeControl_Process(20U);
  assert(AttitudeControl_GetSnapshot().target_dps < 0.0f);
  /* Returning to zero cancels translation immediately, not after a ramp tail. */
  assert(AttitudeControl_SetTrackedDrive(0, 60, 0));
  for (int step = 0; step < 10; ++step) AttitudeControl_Process(20U);
  assert(AttitudeControl_GetSnapshot().forward_rpm > 0);
  assert(AttitudeControl_SetTrackedDrive(0, 0, 0));
  AttitudeControl_Process(20U);
  assert(AttitudeControl_GetSnapshot().forward_rpm == 0);
  line("KEEP\n"); expect("ACK,KEEP\r\n");
  tick += 1501U;
  AttitudeControl_Process(20U);
  assert(!AttitudeControl_GetSnapshot().mode && !SpeedControl_GetSnapshot().enabled);
  line("TRACKMOVE,0,0,0\n"); expect("ERR,TRACKMOVE,ARM_IMU_OR_RANGE\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  line("TRACKMOVE,0,0,0\n"); expect("ACK,TRACKMOVE\r\n");
  gyro_mock.read_failures = 1U;
  AttitudeControl_Process(20U);
  assert(!SpeedControl_GetSnapshot().enabled);
  ++cases;
}

static void test_ps2_diagnostics_and_control_ownership(void)
{
  static const uint8_t raw[] = {0xff, 0x73, 0x5a, 0xff, 0xff, 127, 128, 127, 128};
  static const char *commands[] = {
    "YAW,15\n", "HEAD,90\n", "HEADMOVE,90,30,0\n", "TRACKMOVE,90,30,0\n",
    "MOVE,30,0,0\n", "DRV,30,0,0\n", "SPD,A,30\n"
  };
  unsigned index;
  init();
  memcpy(ps2_mock.raw_frame, raw, sizeof(raw));
  ps2_mock.valid = ps2_mock.connected = 1U;
  ps2_mock.config_attempts = 3U;
  line("PS2RAW\n");
  expect("PS2RAW,FF,73,5A,FF,FF,7F,80,7F,80,1,3\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  ps2_control_mock.enabled = 1U;
  for (index = 0U; index < sizeof(commands) / sizeof(commands[0]); ++index)
  {
    line(commands[index]);
    assert(strstr(transmitted, "PS2") != NULL);
    assert(strncmp(transmitted, "ERR,", 4U) == 0);
  }
  line("STOP\n"); expect("ACK,STOP\r\n");
  assert(!ps2_control_mock.enabled);
  ++cases;
}

static void test_track_frame_sweep_and_split_packets(void)
{
  const char *command = "TRACKMOVE,-179.25,29.34,-17.21\n";
  for (size_t split = 0; split < strlen(command); ++split)
  {
    init();
    line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
    line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
    line("ATEL,0\n"); expect("ACK,ATEL,0\r\n");
    clear_tx();
    if (split) burst(command, split, 1);
    assert(!tx_length);
    burst(command + split, strlen(command) - split, 1);
    expect("ACK,TRACKMOVE\r\n");
    clear_tx(); tick += 400U; Telemetry_Process(); drain();
    assert(!tx_length); /* TRACKMOVE must not turn ATEL back on. */
  }
  init();
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  assert(AttitudeControl_SetTrackedDrive(0, 30, 17));
  for (unsigned step = 0; step < 20; ++step) AttitudeControl_Process(20U);
  for (int angle = -180; angle <= 180; ++angle)
  {
    float radians = (float)angle * 0.01745329252f;
    AttitudeControlSnapshot a;
    gyro_mock.yaw_deg = (float)angle;
    AttitudeControl_Process(20U);
    a = AttitudeControl_GetSnapshot();
    assert(fabsf(a.forward_rpm - (30*cosf(radians)+17*sinf(radians))) <= 0.51f);
    assert(fabsf(a.left_rpm - (-30*sinf(radians)+17*cosf(radians))) <= 0.51f);
    ++cases;
  }
}

/* Validate the installed mixer as a matrix, rather than only three axes.
 * f=sum(w)/4; l=(RF-LF+LR-RR)/4; r=(RF-LF-LR+RR)/4. */
static void test_mecanum_matrix_and_scaling(void)
{
  MecanumWheelTargets targets;
  for (int f = -180; f <= 180; f += 30)
  for (int l = -180; l <= 180; l += 30)
  for (int r = -60; r <= 60; r += 20)
  {
    int raw[4] = {f+l+r, f-l-r, f+l-r, f-l+r};
    int maximum = 0;
    for (int i = 0; i < 4; ++i)
      if (abs(raw[i]) > maximum) maximum = abs(raw[i]);
    Mecanum_CalculateTargets((int16_t)f, (int16_t)l, (int16_t)r, 160U, &targets);
    int wheels[4] = {targets.motor1_right_front, targets.motor2_left_front,
                    targets.motor3_left_rear, targets.motor4_right_rear};
    for (int i = 0; i < 4; ++i)
    {
      assert(abs(wheels[i]) <= 160);
      assert(wheels[i] == (maximum > 160 ? raw[i] * 160 / maximum : raw[i]));
    }
    float scale = maximum > 160 ? 160.0f / maximum : 1.0f;
    assert(fabsf((wheels[0]+wheels[1]+wheels[2]+wheels[3])/4.0f - f*scale) < 1.0f);
    assert(fabsf((wheels[0]-wheels[1]+wheels[2]-wheels[3])/4.0f - l*scale) < 1.0f);
    assert(fabsf((wheels[0]-wheels[1]-wheels[2]+wheels[3])/4.0f - r*scale) < 1.0f);
    ++cases;
  }
  /* One-degree sweep includes every quadrant and non-45-degree direction. */
  for (int degrees = -180; degrees <= 180; ++degrees)
  {
    int f = (int)lroundf(70.0f * cosf(degrees * 0.01745329252f));
    int l = (int)lroundf(70.0f * sinf(degrees * 0.01745329252f));
    Mecanum_CalculateTargets((int16_t)f, (int16_t)l, 0, 160U, &targets);
    assert(targets.motor1_right_front == f+l && targets.motor2_left_front == f-l);
    assert(targets.motor3_left_rear == f+l && targets.motor4_right_rear == f-l);
    ++cases;
  }
}

static void test_tof_query_and_independent_stream(void)
{
  init();
  memset(&tof_mock, 0, sizeof(tof_mock));
  tof_mock.range_status = 255U;
  tof_mock.age_ms = 0xFFFFFFFFU;
  tof_mock.errors = 7U;
  line("TOF\n"); expect("TOF,100,0,0,-1,255,4294967295,7\r\n");
  tof_mock.connected = tof_mock.valid = 1U;
  tof_mock.distance_mm = 1234U;
  tof_mock.range_status = 9U; tof_mock.age_ms = tof_mock.errors = 0U;
  line("TOF\n"); expect("TOF,100,1,1,1234,9,0,0\r\n");
  line("TOF,1\n"); expect("ACK,TOF,1\r\n");
  assert(!SpeedControl_GetSnapshot().enabled && !ChassisTest_GetSnapshot().armed);
  clear_tx(); tick += 99U; Telemetry_Process(); drain(); expect("");
  clear_tx(); tick += 1U; Telemetry_Process(); drain();
  expect("TOF,200,1,1,1234,9,0,0\r\n");
  line("TOFINFO\n"); expect("TOFINFO,LONG,33,40,2,120,0,200,0\r\n");
  /* Combined streams preserve ACK bandwidth: 5 Hz TOF, 2 Hz status. */
  line("TEL,1\n"); expect("ACK,TEL,1\r\n");
  clear_tx(); tick += 199U; Telemetry_Process(); drain(); expect("");
  clear_tx(); tick += 1U; Telemetry_Process(); drain();
  expect("TOF,400,1,1,1234,9,0,0\r\n");
  clear_tx(); tick += 300U; Telemetry_Process(); drain();
  assert(strstr(transmitted, "TOF,700,") && !strstr(transmitted, "TEL,700,"));
  assert(SerialDma_GetTxSnapshot().telemetry_skipped == 1);
  for (unsigned i = 0; i < 5; ++i) { clear_tx(); tick += 100U; Telemetry_Process(); drain(); }
  assert(strstr(transmitted, "TEL,1200,")); /* The staggered stream still gets a turn. */
  line("TEL,0\n"); expect("ACK,TEL,0\r\n");
  line("HLOG,START\n"); expect("HLOG,START,OK\r\n");
  clear_tx(); tick += 500U; Telemetry_Process(); drain(); expect("");
  line("HLOG,STOP\n"); expect("HLOG,STOP,OK\r\n");
  line("HLOG,READ,0,1\n"); expect("HLOG,END,0,0,0\r\n");
  clear_tx(); tick += 500U; Telemetry_Process(); drain(); expect("");
  line("TOF,0\n"); expect("ACK,TOF,0\r\n");
  clear_tx(); tick += 2000U; Telemetry_Process(); drain(); expect("");
}

static void test_invalid_distance_reports_transitions_then_one_hz(void)
{
  init();
  tof_mock.connected = 1U; tof_mock.range_status = 4U;
  line("TOF,1\n"); expect("ACK,TOF,1\r\n");
  clear_tx(); tick += 100; Telemetry_Process(); drain();
  expect("TOF,200,1,0,-1,4,0,0\r\n");
  for (unsigned i = 0; i < 9; ++i)
  { clear_tx(); tick += 100; Telemetry_Process(); drain(); expect(""); }
  clear_tx(); tick += 100; Telemetry_Process(); drain();
  expect("TOF,1200,1,0,-1,4,0,0\r\n");
  tof_mock.range_status = 7;
  clear_tx(); tick += 100; Telemetry_Process(); drain();
  expect("TOF,1300,1,0,-1,7,0,0\r\n");
  tof_mock.valid = 1U; tof_mock.range_status = 9U; tof_mock.distance_mm = 100U;
  clear_tx(); tick += 100; Telemetry_Process(); drain();
  expect("TOF,1400,1,1,100,9,0,0\r\n");
  clear_tx(); tick += 100; Telemetry_Process(); drain();
  expect("TOF,1500,1,1,100,9,0,0\r\n");
}

static void test_periodic_uart_never_queues_ahead_of_replies(void)
{
  init();
  tx_autocomplete = 0U;
  assert(SerialDma_WriteTelemetry("TOF,old\r\n"));
  drain();
  assert(SerialDma_GetTxSnapshot().pending_frames == 1);
  Telemetry_SetTof(1);
  for (unsigned i = 0; i < 20; ++i) { tick += 100; Telemetry_Process(); drain(); }
  assert(SerialDma_GetTxSnapshot().pending_frames == 1);
  assert(SerialDma_GetTxSnapshot().telemetry_skipped == 20);
  assert(SerialDma_Write("ACK,STOP\r\n"));
  assert(SerialDma_GetTxSnapshot().pending_frames == 2);
  HAL_UART_TxCpltCallback(&huart2); drain();
  assert(SerialDma_GetTxSnapshot().pending_frames == 1);
  assert(!strcmp(transmitted, "TOF,old\r\nACK,STOP\r\n"));
  HAL_UART_TxCpltCallback(&huart2); drain();
  assert(!SerialDma_GetTxSnapshot().pending_frames && !SerialDma_GetSnapshot().tx_dropped);
  init();
  line("PING\n"); expect("PONG\r\n");
  tick += 79; assert(!SerialDma_WriteTelemetry("TOF,new\r\n"));
  tick += 1; assert(SerialDma_WriteTelemetry("TOF,new\r\n")); drain();
  line("LINKQ\n"); expect("LINKQ,0,0,1,0\r\n");
}

static void test_parallel_wheel_targets(void)
{
  init();
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  for (unsigned i = 0; i < 4; ++i)
  {
    char command[32];
    snprintf(command, sizeof(command), "WSPD,%c,40\n", 'A' + i);
    line(command); expect("ACK,WSPD\r\n");
  }
  tick += 10; SpeedControl_Process(10);
  SpeedControlSnapshot speed = SpeedControl_GetSnapshot();
  for (unsigned i = 0; i < 4; ++i)
    assert(speed.target_rpm[i] == 40 && motor_applied[i] > 0);
  tick += 301; SpeedControl_Process(10);
  for (unsigned i = 0; i < 4; ++i) assert(motor_applied[i] == 0);
  line("DISARM\n"); expect("ACK,DISARM\r\n");
}

static void test_startup_gains_remain_runtime_tunable(void)
{
  init();
  line("YAWGET\n"); expect("YAWPID,0.1815,1.8062,60,80,20\r\n");
  line("HEADGET\n"); expect("HEADPID,6.0000,1.2500,80,20,D_MEAS\r\n");
  line("YAWPID,0.22,1.5\n"); expect("ACK,YAWPID\r\n");
  line("HEADPID,5,1\n"); expect("ACK,HEADPID\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("YAW,8\n"); expect("ACK,YAW\r\n");
  AttitudeControl_Process(20U);
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("HEAD,10\n"); expect("ACK,HEAD\r\n");
  AttitudeControl_Process(20U);
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("YAWGET\n"); expect("YAWPID,0.2200,1.5000,60,80,20\r\n");
  line("HEADGET\n"); expect("HEADPID,5.0000,1.0000,80,20,D_MEAS\r\n");
}

static void test_front_guard_and_wheel_capture(void)
{
  SpeedControlSnapshot speed;
  init();
  line("WLOG,CLEAR\n"); expect("WLOG,CLEAR,OK\r\n");
  line("WLOG,START\n"); expect("WLOG,START,OK\r\n");
  line("HLOG,CLEAR\n"); expect("ERR,HLOG,STOP_RECORDING_FIRST\r\n");
  tick += 10U; SpeedControl_Process(10U); /* Disabled baseline is a real sample. */
  line("WLOG,STATUS\n"); expect("WLOG,STATUS,1,1,1024,0,1\r\n");
  line("ARM\n"); expect("ACK,ARM,WHEELS_OFF_GROUND\r\n");
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  tof_mock.connected = tof_mock.valid = 1U;
  tof_mock.distance_mm = 201U;
  assert(SpeedControl_SetDrive(160, 0, 0));
  tick += 10U; SpeedControl_Process(10U);
  for (unsigned i = 0; i < 4; ++i) assert(SpeedControl_GetSnapshot().target_rpm[i] == -160);
  tof_mock.distance_mm = 150U;
  tick += 10U; SpeedControl_Process(10U); /* Limit changes without another command. */
  for (unsigned i = 0; i < 4; ++i) assert(SpeedControl_GetSnapshot().target_rpm[i] == -20);
  assert(SpeedControl_GetSnapshot().tof_limited);
  tof_mock.distance_mm = 100U;
  tick += 10U; SpeedControl_Process(10U);
  speed = SpeedControl_GetSnapshot();
  for (unsigned i = 0; i < 4; ++i) assert(!speed.target_rpm[i] && !speed.output_percent[i]);
  assert(SpeedControl_SetDrive(-40, 20, 0));
  tick += 10U; SpeedControl_Process(10U);
  speed = SpeedControl_GetSnapshot();
  assert(speed.target_rpm[0] == 20 && speed.target_rpm[1] == 60);
  assert(!speed.tof_limited); /* Reverse and strafe remain available. */
  assert(SpeedControl_SetDrive(40, 20, 0));
  tick += 10U; SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().target_rpm[0] == -20);
  assert(SpeedControl_GetSnapshot().target_rpm[1] == 20);
  tof_mock.valid = 0U;
  tick += 10U; SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().target_rpm[0] == -60); /* Explicit fail-open policy. */
  tof_mock.valid = 1U; tof_mock.age_ms = 351U;
  tick += 10U; SpeedControl_Process(10U);
  assert(!SpeedControl_GetSnapshot().tof_limited);
  assert(SpeedControl_SetDrive(0, 0, 30));
  tick += 10U; SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 30 &&
         SpeedControl_GetSnapshot().target_rpm[1] == -30); /* Yaw sign unchanged. */
  line("PID,A,1,0.1,0\n"); expect("ERR,PID,STOP_AND_STOP_RECORDING_FIRST\r\n");
  line("WLOG,READ,0,8\n"); expect("ERR,WLOG,MOTION_ACTIVE\r\n");
  line("STOP\n"); expect("ACK,STOP\r\n");
  line("WLOG,STOP\n"); expect("WLOG,STOP,OK\r\n");
  line("WLOG,READ,0,8\n");
  assert(strstr(transmitted, "WLOG,DATA,0,110,110000,0,0,0,") != NULL);
  assert(strstr(transmitted, "WLOG,END,0,8,9\r\n") != NULL);
  line("WLOG,READ,0,9\n"); expect("ERR,LOG,USE_READ,OFFSET,COUNT_0_8\r\n");
  line("WLOG,READ,0,1,JUNK\n"); expect("ERR,LOG,USE_READ,OFFSET,COUNT_0_8\r\n");
  line("HLOG,START\n"); expect("ERR,LOG,USE_CLEAR_FIRST\r\n");
  line("WLOG,CLEAR\n"); expect("WLOG,CLEAR,OK\r\n");
  line("WLOG,START\n"); expect("WLOG,START,OK\r\n");
  for (unsigned i = 0; i < 1025; ++i) { tick += 10U; SpeedControl_Process(10U); }
  line("WLOG,STATUS\n"); expect("WLOG,STATUS,1,1024,1024,1,1025\r\n");
  line("WLOG,STOP\n"); expect("WLOG,STOP,OK\r\n");
  tof_mock.age_ms = 0U;
  line("SPEED,1\n"); expect("ACK,SPEED,1\r\n");
  line("WSPD,A,-60\n"); expect("ACK,WSPD\r\n");
  tick += 10U; SpeedControl_Process(10U);
  speed = SpeedControl_GetSnapshot();
  assert(!speed.target_rpm[0] && !speed.target_rpm[1]); /* No bypass or new wheels. */
  tof_mock.valid = 0U;
  line("WSPD,A,40\n"); expect("ACK,WSPD\r\n");
  tick += 250U; line("WKEEP\n"); expect("ACK,WKEEP\r\n");
  tick += 250U; SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 40);
  tick += 301U; SpeedControl_Process(10U);
  assert(SpeedControl_GetSnapshot().target_rpm[0] == 0);
  line("STOP\n"); expect("ACK,STOP\r\n");
  tof_mock.valid = tof_mock.connected = 1U;
  tof_mock.distance_mm = 150U;
  line("JOG,A,-20,1000\n"); expect("ACK,JOG\r\n");
  assert(motor_applied[0] == -4);
  tick += 10U; SpeedControl_Process(10U);
  assert(motor_applied[0] == -4); /* Disabled recorder cannot cancel JOG. */
  tof_mock.distance_mm = 100U; ChassisTest_Process();
  assert(motor_applied[0] == 0);
  tof_mock.valid = 0U; ChassisTest_Process();
  assert(motor_applied[0] == -20); /* Same explicit fail-open policy. */
  tick += 1001U; ChassisTest_Process();
  assert(motor_applied[0] == 0 && !ChassisTest_IsMotorActive());
}

int main(void)
{
  test_every_split();
  test_stop_restart_and_tel();
  test_bad_lines_and_recovery();
  test_uart_fault_drops_partial_targets();
  test_stack_guard_and_diagnostics();
  test_tunable_gains_and_target_ceiling();
  test_attitude_cascade_and_stop();
  test_rate_and_heading_ram_logs();
  test_tof_query_and_independent_stream();
  test_parallel_wheel_targets();
  test_periodic_uart_never_queues_ahead_of_replies();
  test_invalid_distance_reports_transitions_then_one_hz();
  test_startup_gains_remain_runtime_tunable();
  test_tracked_drive_frame_rest_and_stop();
  test_track_frame_sweep_and_split_packets();
  test_mecanum_installed_numbering();
  test_mecanum_matrix_and_scaling();
  test_ps2_diagnostics_and_control_ownership();
  test_front_guard_and_wheel_capture();
  printf("PASS: %u real-firmware protocol assertions\n", cases);
  return 0;
}
