#include "serial_dma.h"

#include <string.h>

#include "usart.h"

#define SERIAL_PORT_COUNT       2U
#define SERIAL_RX_DMA_SIZE      64U
#define SERIAL_RX_RING_SIZE     256U
#define SERIAL_RX_RING_MASK     (SERIAL_RX_RING_SIZE - 1U)
#define SERIAL_LINE_SIZE        64U
#define SERIAL_TX_QUEUE_DEPTH   8U
#define SERIAL_TX_MESSAGE_SIZE  384U

typedef struct
{
  UART_HandleTypeDef *uart;
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
} SerialPortState;

static SerialPortState serial_ports[SERIAL_PORT_COUNT];
static SerialLineCallback line_callback;

static SerialPortState *SerialDma_FindPort(UART_HandleTypeDef *uart)
{
  uint8_t index;

  for (index = 0U; index < SERIAL_PORT_COUNT; ++index)
  {
    if (serial_ports[index].uart == uart)
    {
      return &serial_ports[index];
    }
  }
  return NULL;
}

static uint8_t SerialDma_StartRx(SerialPortState *port)
{
  HAL_StatusTypeDef status;

  status = HAL_UARTEx_ReceiveToIdle_DMA(port->uart, port->rx_dma_buffer,
                                       SERIAL_RX_DMA_SIZE);
  if (status != HAL_OK)
  {
    return 0U;
  }

  __HAL_DMA_DISABLE_IT(port->uart->hdmarx, DMA_IT_HT);
  port->rx_restart_needed = 0U;
  return 1U;
}

static void SerialDma_PushRx(SerialPortState *port, const uint8_t *data,
                             uint16_t length)
{
  uint16_t index;
  uint16_t next;

  for (index = 0U; index < length; ++index)
  {
    next = (port->rx_head + 1U) & SERIAL_RX_RING_MASK;
    if (next == port->rx_tail)
    {
      break;
    }
    port->rx_ring[port->rx_head] = data[index];
    port->rx_head = next;
  }
}

static void SerialDma_StartTx(SerialPortState *port)
{
  if ((port->tx_busy != 0U) || (port->tx_tail == port->tx_head))
  {
    return;
  }

  if (HAL_UART_Transmit_DMA(port->uart, port->tx_queue[port->tx_tail],
                            port->tx_length[port->tx_tail]) == HAL_OK)
  {
    port->tx_busy = 1U;
  }
}

static void SerialDma_ProcessByte(SerialPortState *port, uint8_t byte)
{
  if (byte == '\r')
  {
    return;
  }

  if (byte == '\n')
  {
    if (port->rx_line_length == 0U)
    {
      return;
    }
    port->rx_line[port->rx_line_length] = '\0';
    if (line_callback != NULL)
    {
      line_callback(port->rx_line);
    }
    port->rx_line_length = 0U;
    return;
  }

  if ((byte < 0x20U) || (byte > 0x7EU))
  {
    return;
  }

  if (port->rx_line_length >= (SERIAL_LINE_SIZE - 1U))
  {
    port->rx_line_length = 0U;
    SerialDma_Write("ERR,LINE_TOO_LONG\r\n");
    return;
  }

  port->rx_line[port->rx_line_length++] = (char)byte;
}

static uint8_t SerialDma_QueueWrite(SerialPortState *port, const char *text,
                                    size_t length)
{
  uint8_t next;

  next = (uint8_t)((port->tx_head + 1U) % SERIAL_TX_QUEUE_DEPTH);
  if (next == port->tx_tail)
  {
    return 0U;
  }

  memcpy(port->tx_queue[port->tx_head], text, length);
  port->tx_length[port->tx_head] = (uint16_t)length;
  port->tx_head = next;
  return 1U;
}

uint8_t SerialDma_Init(SerialLineCallback callback)
{
  uint8_t ready = 1U;
  uint8_t index;

  line_callback = callback;
  memset(serial_ports, 0, sizeof(serial_ports));
  serial_ports[0].uart = &huart2;
  serial_ports[1].uart = &huart3;

  for (index = 0U; index < SERIAL_PORT_COUNT; ++index)
  {
    if (SerialDma_StartRx(&serial_ports[index]) == 0U)
    {
      ready = 0U;
      serial_ports[index].rx_restart_needed = 1U;
    }
  }
  return ready;
}

void SerialDma_Process(void)
{
  uint8_t index;
  uint8_t byte;
  SerialPortState *port;

  for (index = 0U; index < SERIAL_PORT_COUNT; ++index)
  {
    port = &serial_ports[index];
    if (port->rx_restart_needed != 0U)
    {
      (void)HAL_UART_AbortReceive(port->uart);
      (void)SerialDma_StartRx(port);
    }

    while (port->rx_tail != port->rx_head)
    {
      byte = port->rx_ring[port->rx_tail];
      port->rx_tail = (port->rx_tail + 1U) & SERIAL_RX_RING_MASK;
      SerialDma_ProcessByte(port, byte);
    }

    SerialDma_StartTx(port);
  }
}

uint8_t SerialDma_Write(const char *text)
{
  uint8_t index;
  uint8_t queued = 0U;
  size_t length;

  if (text == NULL)
  {
    return 0U;
  }

  length = strlen(text);
  if ((length == 0U) || (length >= SERIAL_TX_MESSAGE_SIZE))
  {
    return 0U;
  }

  for (index = 0U; index < SERIAL_PORT_COUNT; ++index)
  {
    if (SerialDma_QueueWrite(&serial_ports[index], text, length) != 0U)
    {
      queued = 1U;
    }
  }
  return queued;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  SerialPortState *port = SerialDma_FindPort(huart);

  if (port == NULL)
  {
    return;
  }

  SerialDma_PushRx(port, port->rx_dma_buffer, size);
  if (SerialDma_StartRx(port) == 0U)
  {
    port->rx_restart_needed = 1U;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  SerialPortState *port = SerialDma_FindPort(huart);

  if (port == NULL)
  {
    return;
  }

  port->tx_tail = (uint8_t)((port->tx_tail + 1U) % SERIAL_TX_QUEUE_DEPTH);
  port->tx_busy = 0U;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  SerialPortState *port = SerialDma_FindPort(huart);

  if (port != NULL)
  {
    port->rx_restart_needed = 1U;
  }
}
