#include "bluetooth_app.h"
#include "motor_app.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>

volatile uint32_t bluetooth_rx_count = 0UL;
volatile uint8_t bluetooth_last_byte = 0U;
volatile uint32_t bluetooth_error_count = 0UL;

#define BLUETOOTH_LINE_SIZE  48U
static char bluetooth_line[BLUETOOTH_LINE_SIZE];
static uint8_t bluetooth_rx_byte;
static volatile uint8_t bluetooth_line_length;
static volatile uint8_t bluetooth_line_ready;
static uint8_t bluetooth_line_position;
static uint8_t bluetooth_ready_text[] = {
  'B', 'T', ' ', 'R', 'E', 'A', 'D', 'Y', '\r', '\n'
};
static uint8_t bluetooth_bad_command[] = {
  'E', 'R', 'R', ',', 'B', 'A', 'D', '_', 'C', 'M', 'D', '\r', '\n'
};
static char bluetooth_status_format[] =
  "STAT,%ld,%ld,%ld,%ld,%u,%lu,%u,%lu,%u,%lu\r\n";
static char bluetooth_ok_format[] = "OK,SET,%ld,%ld\r\n";

static void bluetooth_send_status(void)
{
  uint8_t tx_data[96];
  int32_t actual_1;
  int32_t actual_2;
  int32_t target_1;
  int32_t target_2;
  int length;

  actual_1 = (int32_t)(motor_angle[0] * 10.0f +
             ((motor_angle[0] < 0.0f) ? -0.5f : 0.5f));
  actual_2 = (int32_t)(motor_angle[1] * 10.0f +
             ((motor_angle[1] < 0.0f) ? -0.5f : 0.5f));
  target_1 = (int32_t)(motor_target_angle[0] * 10.0f +
             ((motor_target_angle[0] < 0.0f) ? -0.5f : 0.5f));
  target_2 = (int32_t)(motor_target_angle[1] * 10.0f +
             ((motor_target_angle[1] < 0.0f) ? -0.5f : 0.5f));

  length = sprintf((char *)tx_data, bluetooth_status_format,
                   (long)actual_1, (long)actual_2,
                   (long)target_1, (long)target_2, motor_state,
                   (unsigned long)bluetooth_rx_count,
                   (unsigned int)bluetooth_last_byte,
                   (unsigned long)bluetooth_error_count,
                   (unsigned int)huart2.RxState,
                   (unsigned long)huart2.Instance->SR);
  HAL_UART_Transmit(&huart2, tx_data, (uint16_t)length, 100U);
}

static void bluetooth_process_line(char *line)
{
  uint8_t tx_data[40];
  char *end;
  long angle_1;
  long angle_2;
  int length;

  if ((line[0] == 'S') && (line[1] == 'E') &&
      (line[2] == 'T') && (line[3] == ','))
  {
    angle_1 = strtol(&line[4], &end, 10);
    if (*end == ',')
    {
      angle_2 = strtol(end + 1, &end, 10);
      if ((*end == '\0') && (angle_1 >= -3600L) && (angle_1 <= 3600L) &&
          (angle_2 >= -3600L) && (angle_2 <= 3600L) &&
          (motor_set_angles((int16_t)angle_1, (int16_t)angle_2) != 0U))
      {
        length = sprintf((char *)tx_data, bluetooth_ok_format,
                         angle_1, angle_2);
        HAL_UART_Transmit(&huart2, tx_data, (uint16_t)length, 100U);
        return;
      }
    }
  }
  else if ((line[0] == 'S') && (line[1] == 'T') &&
           (line[2] == 'A') && (line[3] == 'T') &&
           (line[4] == '?') && (line[5] == '\0'))
  {
    bluetooth_send_status();
    return;
  }

  ++bluetooth_error_count;
  HAL_UART_Transmit(&huart2, bluetooth_bad_command,
                    sizeof(bluetooth_bad_command), 100U);
}

void bluetooth_init(void)
{
  bluetooth_rx_count = 0UL;
  bluetooth_last_byte = 0U;
  bluetooth_error_count = 0UL;
  bluetooth_line_length = 0U;
  bluetooth_line_ready = 0U;
  bluetooth_line_position = 0U;

  __HAL_UART_CLEAR_OREFLAG(&huart2);
  if (HAL_UART_Receive_IT(&huart2, &bluetooth_rx_byte, 1U) != HAL_OK)
  {
    ++bluetooth_error_count;
  }

  HAL_UART_Transmit(&huart2, bluetooth_ready_text,
                    sizeof(bluetooth_ready_text), 100U);
}

void bluetooth_pro(void)
{
  static uint32_t send_time = 0UL;
  char line[BLUETOOTH_LINE_SIZE];
  uint8_t length;
  uint8_t i;

  if (bluetooth_line_ready != 0U)
  {
    __disable_irq();
    length = bluetooth_line_length;
    for (i = 0U; i <= length; ++i)
    {
      line[i] = bluetooth_line[i];
    }
    bluetooth_line_ready = 0U;
    __enable_irq();

    bluetooth_process_line(line);
  }

  if ((uint32_t)(HAL_GetTick() - send_time) < 1000UL)
  {
    return;
  }
  send_time = HAL_GetTick();
  bluetooth_send_status();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t data;

  if (huart->Instance != USART2)
  {
    return;
  }

  data = bluetooth_rx_byte;
  bluetooth_last_byte = data;
  ++bluetooth_rx_count;

  if (bluetooth_line_ready == 0U)
  {
    if ((data == '\n') && (bluetooth_line_position != 0U))
    {
      bluetooth_line[bluetooth_line_position] = '\0';
      bluetooth_line_length = bluetooth_line_position;
      bluetooth_line_position = 0U;
      bluetooth_line_ready = 1U;
    }
    else if (data == '\r')
    {
      /* 忽略回车，使用换行作为一条命令的结束。 */
    }
    else if (bluetooth_line_position < (BLUETOOTH_LINE_SIZE - 1U))
    {
      bluetooth_line[bluetooth_line_position] = (char)data;
      ++bluetooth_line_position;
    }
    else
    {
      bluetooth_line_position = 0U;
      ++bluetooth_error_count;
    }
  }

  /* 与 diansai_test 相同：每收到一个字节，立即启动下一字节接收。 */
  if (HAL_UART_Receive_IT(&huart2, &bluetooth_rx_byte, 1U) != HAL_OK)
  {
    ++bluetooth_error_count;
  }
}

void bluetooth_uart_error(void)
{
  ++bluetooth_error_count;
  __HAL_UART_CLEAR_OREFLAG(&huart2);
  (void)HAL_UART_Receive_IT(&huart2, &bluetooth_rx_byte, 1U);
}
