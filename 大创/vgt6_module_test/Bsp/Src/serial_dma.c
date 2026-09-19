#include "serial_dma.h"

#include <stddef.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "usart.h"

#define SERIAL_RX_DMA_SIZE    64U
#define SERIAL_RX_RING_SIZE   256U
#define SERIAL_RX_RING_MASK   (SERIAL_RX_RING_SIZE - 1U)
#define SERIAL_LINE_SIZE      64U
#define SERIAL_TX_QUEUE_DEPTH 16U
#define SERIAL_TX_MESSAGE_SIZE 256U

typedef struct
{
  uint8_t rx_dma_buffer[SERIAL_RX_DMA_SIZE];
  uint8_t rx_ring[SERIAL_RX_RING_SIZE];
  volatile uint16_t rx_head;
  volatile uint16_t rx_tail;
  volatile uint8_t rx_restart_needed;
  char rx_line[SERIAL_LINE_SIZE];
  uint8_t rx_line_length;
  uint8_t tx_queue[SERIAL_TX_QUEUE_DEPTH][SERIAL_TX_MESSAGE_SIZE];
  uint16_t tx_length[SERIAL_TX_QUEUE_DEPTH];
  volatile uint8_t tx_head;
  volatile uint8_t tx_tail;
  volatile uint8_t tx_busy;
  volatile uint8_t ready;
} SerialPortState;

static SerialPortState serial_port;
static SerialLineCallback line_callback;

static uint8_t SerialDma_StartRx(void)
{
  HAL_StatusTypeDef status;

  status = HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial_port.rx_dma_buffer,
                                        SERIAL_RX_DMA_SIZE);
  if (status != HAL_OK)
  {
    serial_port.ready = 0U;
    return 0U;
  }
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  serial_port.rx_restart_needed = 0U;
  serial_port.ready = 1U;
  return 1U;
}

static void SerialDma_PushRx(const uint8_t *data, uint16_t length)
{
  uint16_t index;
  uint16_t next;

  for (index = 0U; index < length; ++index)
  {
    next = (uint16_t)((serial_port.rx_head + 1U) &
                      SERIAL_RX_RING_MASK);
    if (next == serial_port.rx_tail)
    {
      break;
    }
    serial_port.rx_ring[serial_port.rx_head] = data[index];
    serial_port.rx_head = next;
  }
}

static void SerialDma_StartTx(void)
{
  if ((serial_port.tx_busy != 0U) ||
      (serial_port.tx_tail == serial_port.tx_head))
  {
    return;
  }
  if (HAL_UART_Transmit_DMA(&huart2,
                            serial_port.tx_queue[serial_port.tx_tail],
                            serial_port.tx_length[serial_port.tx_tail]) == HAL_OK)
  {
    serial_port.tx_busy = 1U;
  }
}

static void SerialDma_ProcessByte(uint8_t byte)
{
  if ((byte == '\r') || (byte == '\n'))
  {
    if (serial_port.rx_line_length == 0U)
    {
      return;
    }
    serial_port.rx_line[serial_port.rx_line_length] = '\0';
    if (line_callback != NULL)
    {
      line_callback(serial_port.rx_line);
    }
    serial_port.rx_line_length = 0U;
    return;
  }
  if ((byte < 0x20U) || (byte > 0x7EU))
  {
    return;
  }
  if (serial_port.rx_line_length >= (SERIAL_LINE_SIZE - 1U))
  {
    serial_port.rx_line_length = 0U;
    (void)SerialDma_Write("ERR,LINE_TOO_LONG\r\n");
    return;
  }
  serial_port.rx_line[serial_port.rx_line_length++] = (char)byte;
}

static uint8_t SerialDma_QueueWrite(const char *text, size_t length)
{
  uint8_t next;

  next = (uint8_t)((serial_port.tx_head + 1U) % SERIAL_TX_QUEUE_DEPTH);
  if (next == serial_port.tx_tail)
  {
    return 0U;
  }
  memcpy(serial_port.tx_queue[serial_port.tx_head], text, length);
  serial_port.tx_length[serial_port.tx_head] = (uint16_t)length;
  serial_port.tx_head = next;
  return 1U;
}

uint8_t SerialDma_Init(SerialLineCallback callback)
{
  memset(&serial_port, 0, sizeof(serial_port));
  line_callback = callback;
  return SerialDma_StartRx();
}

void SerialDma_Process(void)
{
  uint8_t byte;

  if (serial_port.rx_restart_needed != 0U)
  {
    (void)HAL_UART_AbortReceive(&huart2);
    (void)SerialDma_StartRx();
  }
  while (serial_port.rx_tail != serial_port.rx_head)
  {
    byte = serial_port.rx_ring[serial_port.rx_tail];
    serial_port.rx_tail = (uint16_t)((serial_port.rx_tail + 1U) &
                                     SERIAL_RX_RING_MASK);
    SerialDma_ProcessByte(byte);
  }
  SerialDma_StartTx();
}

uint8_t SerialDma_Write(const char *text)
{
  size_t length;

  if ((text == NULL) || (serial_port.ready == 0U))
  {
    return 0U;
  }
  length = strlen(text);
  if ((length == 0U) || (length >= SERIAL_TX_MESSAGE_SIZE))
  {
    return 0U;
  }
  return SerialDma_QueueWrite(text, length);
}

uint8_t SerialDma_IsReady(void)
{
  return serial_port.ready;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  uint8_t line_end;

  if (huart != &huart2)
  {
    return;
  }
  if (size > SERIAL_RX_DMA_SIZE)
  {
    size = SERIAL_RX_DMA_SIZE;
  }
  SerialDma_PushRx(serial_port.rx_dma_buffer, size);
  /* ReceiveToIdle is also used as a command terminator. This accepts
   * terminals that send "ARM" or "A+" without CR/LF, while CR/LF input
   * remains fully supported. A full DMA buffer is left untouched so a long
   * line can continue in the normal ring-buffer path. */
  if ((size > 0U) && (size < SERIAL_RX_DMA_SIZE) &&
      (serial_port.rx_dma_buffer[size - 1U] != '\r') &&
      (serial_port.rx_dma_buffer[size - 1U] != '\n'))
  {
    line_end = '\n';
    SerialDma_PushRx(&line_end, 1U);
  }
  if (SerialDma_StartRx() == 0U)
  {
    serial_port.rx_restart_needed = 1U;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart != &huart2)
  {
    return;
  }
  serial_port.tx_tail = (uint8_t)((serial_port.tx_tail + 1U) %
                                  SERIAL_TX_QUEUE_DEPTH);
  serial_port.tx_busy = 0U;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart2)
  {
    serial_port.rx_restart_needed = 1U;
    serial_port.ready = 0U;
  }
}
