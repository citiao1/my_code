#include "motor_test.h"

#include "main.h"
#include "tim.h"
#include "usart.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MOTOR_TEST_DUTY_PERCENT  10U
#define MOTOR_TEST_RUN_MS        1000U
#define MOTOR_TEST_TELEMETRY_MS  50U
#define MOTOR_TEST_RX_SIZE       12U

typedef struct
{
  char name;
  TIM_HandleTypeDef *input_1_timer;
  uint32_t input_1_channel;
  TIM_HandleTypeDef *input_2_timer;
  uint32_t input_2_channel;
  TIM_HandleTypeDef *encoder_timer;
  int8_t drive_sign;
  int8_t encoder_sign;
} MotorTest_Channel;

/* Calibrated so command '+' is forward and forward encoder counts are positive. */
static const MotorTest_Channel motor_channels[] =
{
  {'A', &htim10, TIM_CHANNEL_1, &htim11, TIM_CHANNEL_1, &htim2, -1, -1},
  {'B', &htim9,  TIM_CHANNEL_1, &htim9,  TIM_CHANNEL_2, &htim3, -1,  1},
  {'C', &htim1,  TIM_CHANNEL_1, &htim1,  TIM_CHANNEL_2, &htim4,  1, -1},
  {'D', &htim1,  TIM_CHANNEL_3, &htim1,  TIM_CHANNEL_4, &htim5,  1,  1}
};

static char rx_line[MOTOR_TEST_RX_SIZE];
static uint8_t rx_length;
static int8_t active_motor = -1;
static int8_t active_direction;
static uint32_t run_start_tick;
static uint32_t telemetry_tick;
static uint32_t stop_tick;
static uint8_t motor_test_ready;

static void UartWrite(const char *text)
{
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)text, (uint16_t)strlen(text), 500U);
}

static void StopAllOutputs(void)
{
  uint32_t index;

  for (index = 0U; index < (sizeof(motor_channels) / sizeof(motor_channels[0])); ++index)
  {
    __HAL_TIM_SET_COMPARE(motor_channels[index].input_1_timer,
                          motor_channels[index].input_1_channel, 0U);
    __HAL_TIM_SET_COMPARE(motor_channels[index].input_2_timer,
                          motor_channels[index].input_2_channel, 0U);
  }
}

static int32_t ReadEncoder(const MotorTest_Channel *motor)
{
  uint32_t count = __HAL_TIM_GET_COUNTER(motor->encoder_timer);
  int32_t signed_count;

  if ((motor->encoder_timer->Instance == TIM3) ||
      (motor->encoder_timer->Instance == TIM4))
  {
    signed_count = (int32_t)(int16_t)count;
  }
  else
  {
    signed_count = (int32_t)count;
  }

  return signed_count * (int32_t)motor->encoder_sign;
}

static void SendTelemetry(uint32_t now)
{
  char message[48];
  const MotorTest_Channel *motor;

  if (active_motor < 0)
  {
    return;
  }

  motor = &motor_channels[(uint8_t)active_motor];
  (void)snprintf(message, sizeof(message), "TEL,%lu,%c,%d,%ld,%u\r\n",
                 (unsigned long)(now - run_start_tick), motor->name,
                 (int)active_direction, (long)ReadEncoder(motor),
                 (unsigned int)MOTOR_TEST_DUTY_PERCENT);
  UartWrite(message);
}

static void SetMotorOutput(const MotorTest_Channel *motor, int8_t direction)
{
  uint32_t period = __HAL_TIM_GET_AUTORELOAD(motor->input_1_timer) + 1U;
  uint32_t drive = (period * MOTOR_TEST_DUTY_PERCENT) / 100U;
  int8_t hardware_direction = direction * motor->drive_sign;

  StopAllOutputs();

  if (hardware_direction > 0)
  {
    __HAL_TIM_SET_COMPARE(motor->input_1_timer, motor->input_1_channel, period - drive);
    __HAL_TIM_SET_COMPARE(motor->input_2_timer, motor->input_2_channel, period);
  }
  else
  {
    __HAL_TIM_SET_COMPARE(motor->input_1_timer, motor->input_1_channel, period);
    __HAL_TIM_SET_COMPARE(motor->input_2_timer, motor->input_2_channel, period - drive);
  }
}

static uint8_t StartOutputsAndEncoders(void)
{
  HAL_StatusTypeDef status = HAL_OK;

  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  status |= HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
  status |= HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
  status |= HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
  status |= HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1);

  status |= HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  status |= HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  status |= HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
  status |= HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);

  StopAllOutputs();
  return (status == HAL_OK) ? 1U : 0U;
}

static void FinishTimedRun(void)
{
  char message[48];
  const MotorTest_Channel *motor;
  int32_t encoder_count;

  if (active_motor < 0)
  {
    return;
  }

  motor = &motor_channels[(uint8_t)active_motor];
  StopAllOutputs();
  encoder_count = ReadEncoder(motor);
  SendTelemetry(HAL_GetTick());
  (void)snprintf(message, sizeof(message), "DONE %c%c ENC=%ld\r\n",
                 motor->name, (active_direction > 0) ? '+' : '-', (long)encoder_count);
  active_motor = -1;
  UartWrite(message);
}

static void StopForEnableSwitch(void)
{
  StopAllOutputs();
  active_motor = -1;
  UartWrite("STOP: MOTOR_EN(PD3)=LOW\r\n");
}

static void ExecuteCommand(char *command)
{
  char message[40];
  uint8_t index;
  size_t length = strlen(command);

  while ((length > 0U) && (command[length - 1U] == ' '))
  {
    command[--length] = '\0';
  }

  if ((strcmp(command, "STOP") == 0) || (strcmp(command, "S") == 0))
  {
    StopAllOutputs();
    active_motor = -1;
    UartWrite("STOP\r\n");
    return;
  }

  if ((strcmp(command, "HELP") == 0) || (strcmp(command, "?") == 0))
  {
    UartWrite("A+/A-/B+/B-/C+/C-/D+/D-/STOP + Enter\r\n");
    return;
  }

  if ((length != 2U) || ((command[1] != '+') && (command[1] != '-')))
  {
    UartWrite("ERR: use HELP\r\n");
    return;
  }

  for (index = 0U; index < (sizeof(motor_channels) / sizeof(motor_channels[0])); ++index)
  {
    if (command[0] == motor_channels[index].name)
    {
      if (motor_test_ready == 0U)
      {
        UartWrite("BLOCKED: timer start failed\r\n");
        return;
      }

      if (HAL_GPIO_ReadPin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin) != GPIO_PIN_SET)
      {
        StopAllOutputs();
        active_motor = -1;
        UartWrite("BLOCKED: MOTOR_EN(PD3)=LOW\r\n");
        return;
      }

      __HAL_TIM_SET_COUNTER(motor_channels[index].encoder_timer, 0U);
      active_motor = (int8_t)index;
      active_direction = (command[1] == '+') ? 1 : -1;
      SetMotorOutput(&motor_channels[index], active_direction);
      run_start_tick = HAL_GetTick();
      telemetry_tick = run_start_tick;
      stop_tick = run_start_tick + MOTOR_TEST_RUN_MS;
      (void)snprintf(message, sizeof(message), "RUN %c%c 10%% 1000ms\r\n",
                     motor_channels[index].name, (active_direction > 0) ? '+' : '-');
      UartWrite(message);
      return;
    }
  }

  UartWrite("ERR: unknown motor\r\n");
}

static void ReceiveCommands(void)
{
  uint8_t byte;

  while (HAL_UART_Receive(&huart2, &byte, 1U, 0U) == HAL_OK)
  {
    if ((byte == '\r') || (byte == '\n'))
    {
      if (rx_length > 0U)
      {
        rx_line[rx_length] = '\0';
        ExecuteCommand(rx_line);
        rx_length = 0U;
      }
    }
    else if ((byte == 0x08U) || (byte == 0x7FU))
    {
      if (rx_length > 0U)
      {
        --rx_length;
      }
    }
    else if ((byte >= 0x20U) && (byte <= 0x7EU))
    {
      if ((byte >= 'a') && (byte <= 'z'))
      {
        byte = (uint8_t)(byte - ('a' - 'A'));
      }

      if (rx_length < (MOTOR_TEST_RX_SIZE - 1U))
      {
        rx_line[rx_length++] = (char)byte;
      }
      else
      {
        rx_length = 0U;
        UartWrite("ERR: command too long\r\n");
      }
    }
  }
}

void MotorTest_Init(void)
{
  StopAllOutputs();
  motor_test_ready = StartOutputsAndEncoders();

  UartWrite("MECANUM MOTOR TEST READY\r\n");
  UartWrite("A=RR B=LR C=RF D=LF; +=forward -=backward\r\n");
  UartWrite("A+/A-/B+/B-/C+/C-/D+/D-/STOP + Enter\r\n");

  if (motor_test_ready == 0U)
  {
    UartWrite("INIT ERROR: motor outputs remain stopped\r\n");
  }
}

void MotorTest_Process(void)
{
  if (active_motor >= 0)
  {
    if (HAL_GPIO_ReadPin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin) != GPIO_PIN_SET)
    {
      StopForEnableSwitch();
    }
    else if ((int32_t)(HAL_GetTick() - stop_tick) >= 0)
    {
      FinishTimedRun();
    }
    else if ((int32_t)(HAL_GetTick() - telemetry_tick) >= 0)
    {
      SendTelemetry(HAL_GetTick());
      telemetry_tick += MOTOR_TEST_TELEMETRY_MS;
    }
  }

  ReceiveCommands();
}
