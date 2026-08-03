#include "motor_uart.h"
#include "bluetooth_app.h"
#include "usart.h"

#define UART_RX_SIZE  32U

float uart_angle[2] = {0.0f, 0.0f};
uint8_t uart_position_ready[2] = {0U, 0U};
volatile uint32_t uart_error_count = 0UL;

static uint8_t dma_rx_data[UART_RX_SIZE];
static uint8_t rx_data[UART_RX_SIZE];
static volatile uint16_t rx_len;
static volatile uint8_t rx_flag;
static volatile uint8_t rx_restart;

void motor_uart_init_internal(void)
{
  rx_len = 0U;
  rx_flag = 0U;
  rx_restart = 0U;
  uart_position_ready[0] = 0U;
  uart_position_ready[1] = 0U;
  uart_error_count = 0UL;

  __HAL_UART_CLEAR_OREFLAG(&huart4);
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart4, dma_rx_data, UART_RX_SIZE) == HAL_OK)
  {
    __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);
  }
  else
  {
    rx_restart = 1U;
  }
}

void motor_uart_process_internal(void)
{
  uint8_t data[UART_RX_SIZE];
  uint16_t len;
  uint16_t i;
  uint8_t index;
  uint32_t position;

  if (rx_restart != 0U)
  {
    (void)HAL_UART_AbortReceive(&huart4);
    __HAL_UART_CLEAR_OREFLAG(&huart4);
    rx_restart = 0U;
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart4, dma_rx_data, UART_RX_SIZE) == HAL_OK)
    {
      __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);
    }
    else
    {
      rx_restart = 1U;
    }
  }

  if (rx_flag == 0U)
  {
    return;
  }

  /* 中断只负责收数据，完整的数据帧在主循环中解析。 */
  __disable_irq();
  len = rx_len;
  for (i = 0U; i < len; ++i)
  {
    data[i] = rx_data[i];
  }
  rx_flag = 0U;
  __enable_irq();

  /* 普通命令的 4 字节应答无需处理，只提取 S_CPOS 的 8 字节位置帧。 */
  for (i = 0U; (i + 7U) < len; ++i)
  {
    if (((data[i] == 1U) || (data[i] == 2U)) &&
        (data[i + 1U] == 0x36U) &&
        (data[i + 7U] == 0x6BU))
    {
      index = (data[i] == 1U) ? 0U : 1U;
      position = ((uint32_t)data[i + 3U] << 24) |
                 ((uint32_t)data[i + 4U] << 16) |
                 ((uint32_t)data[i + 5U] << 8) |
                 (uint32_t)data[i + 6U];
      uart_angle[index] = ((float)position * 360.0f) / 65536.0f;
      if (data[i + 2U] != 0U)
      {
        uart_angle[index] = -uart_angle[index];
      }
      uart_position_ready[index] = 1U;
      i += 7U;
    }
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  uint16_t i;

  if (huart->Instance != UART4)
  {
    return;
  }

  if (size > UART_RX_SIZE)
  {
    size = UART_RX_SIZE;
  }

  if (rx_flag == 0U)
  {
    for (i = 0U; i < size; ++i)
    {
      rx_data[i] = dma_rx_data[i];
    }
    rx_len = size;
    rx_flag = 1U;
  }
  else
  {
    ++uart_error_count;
  }

  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart4, dma_rx_data, UART_RX_SIZE) == HAL_OK)
  {
    __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);
  }
  else
  {
    rx_restart = 1U;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == UART4)
  {
    ++uart_error_count;
    rx_restart = 1U;
  }
  else if (huart->Instance == USART2)
  {
    bluetooth_uart_error();
  }
}
