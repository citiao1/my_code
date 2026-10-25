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
  volatile uint8_t rx_overflow;
  volatile uint8_t rx_fault;
  char rx_line[SERIAL_LINE_SIZE];
  uint8_t rx_line_length;
  uint8_t rx_discard_line;
  uint8_t tx_queue[SERIAL_TX_QUEUE_DEPTH][SERIAL_TX_MESSAGE_SIZE];
  uint16_t tx_length[SERIAL_TX_QUEUE_DEPTH];
  volatile uint8_t tx_head;
  volatile uint8_t tx_tail;
  volatile uint8_t tx_busy;
  volatile uint8_t ready;
} SerialPortState;

static SerialPortState serial_port;
static SerialLineCallback line_callback;
static volatile SerialDmaSnapshot serial_stats;
static volatile SerialUartSnapshot uart_stats;
static uint32_t last_command_tick, telemetry_skipped;
static uint8_t command_seen;

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
      serial_port.rx_overflow = 1U;
      ++serial_stats.rx_overflows;
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
  /* Publish ownership before starting DMA; completion can preempt this call. */
  serial_port.tx_busy = 1U;
  if (HAL_UART_Transmit_DMA(&huart2,
                            serial_port.tx_queue[serial_port.tx_tail],
                            serial_port.tx_length[serial_port.tx_tail]) != HAL_OK)
  {
    serial_port.tx_busy = 0U;
  }
}

static void SerialDma_ProcessByte(uint8_t byte)
{
  if ((byte == '\r') || (byte == '\n'))
  {
    if (serial_port.rx_discard_line != 0U)
    {
      serial_port.rx_discard_line = 0U;
      serial_port.rx_line_length = 0U;
      return;
    }
    if (serial_port.rx_line_length == 0U)
    {
      return;
    }
    serial_port.rx_line[serial_port.rx_line_length] = '\0';
    if (line_callback != NULL)
    {
      ++serial_stats.rx_lines;
      last_command_tick = HAL_GetTick();
      command_seen = 1U;
      line_callback(serial_port.rx_line);
    }
    serial_port.rx_line_length = 0U;
    return;
  }
  if (serial_port.rx_discard_line != 0U)
  {
    return;
  }
  if ((byte < 0x20U) || (byte > 0x7EU))
  {
    serial_port.rx_discard_line = 1U;
    serial_port.rx_line_length = 0U;
    (void)SerialDma_Write("ERR,NON_ASCII\r\n");
    return;
  }
  if (serial_port.rx_line_length >= (SERIAL_LINE_SIZE - 1U))
  {
    serial_port.rx_line_length = 0U;
    serial_port.rx_discard_line = 1U;
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
    ++serial_stats.tx_dropped;
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
  serial_stats.rx_bytes = 0U;
  serial_stats.rx_lines = 0U;
  serial_stats.rx_overflows = 0U;
  serial_stats.tx_dropped = 0U;
  serial_stats.uart_errors = 0U;
  uart_stats.last_error = 0U;
  uart_stats.parity_errors = 0U;
  uart_stats.noise_errors = 0U;
  uart_stats.framing_errors = 0U;
  uart_stats.overrun_errors = 0U;
  uart_stats.dma_errors = 0U;
  last_command_tick = telemetry_skipped = 0U;
  command_seen = 0U;
  line_callback = callback;
  return SerialDma_StartRx();
}

void SerialDma_Process(void)
{
  uint8_t byte;

  if (serial_port.rx_restart_needed != 0U)
  {
    (void)HAL_UART_AbortReceive(&huart2);
    if (serial_port.rx_fault != 0U)
    {
      /* Never combine bytes from before/after a hardware reception error.
       * The bridge prefixes every command with LF to resynchronize safely. */
      serial_port.rx_tail = serial_port.rx_head;
      serial_port.rx_line_length = 0U;
      serial_port.rx_discard_line = 1U;
    }
    (void)SerialDma_StartRx();
    if ((serial_port.ready != 0U) && (serial_port.rx_fault != 0U))
    {
      serial_port.rx_fault = 0U;
      (void)SerialDma_Write("ERR,UART_RX,QUERY_UART\r\n");
    }
  }
  if (serial_port.rx_overflow != 0U)
  {
    serial_port.rx_line_length = 0U;
    serial_port.rx_discard_line = 1U;
    serial_port.rx_overflow = 0U;
    (void)SerialDma_Write("ERR,RX_OVERFLOW\r\n");
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
    ++serial_stats.tx_dropped;
    return 0U;
  }
  return SerialDma_QueueWrite(text, length);
}

uint8_t SerialDma_CanWriteTelemetry(void)
{
  return serial_port.ready &&
      (!command_seen || HAL_GetTick() - last_command_tick >= 80U) &&
      serial_port.tx_head == serial_port.tx_tail;
}

uint8_t SerialDma_WriteTelemetry(const char *text)
{
  /* A query/target/heartbeat owns the UART first. Keep at most one periodic
   * frame in flight; obsolete live readings are skipped, never replayed. */
  if (!SerialDma_CanWriteTelemetry())
  {
    ++telemetry_skipped;
    return 0U;
  }
  return SerialDma_Write(text);
}

SerialTxSnapshot SerialDma_GetTxSnapshot(void)
{
  SerialTxSnapshot snapshot = {0U, 0U, telemetry_skipped,
      command_seen ? HAL_GetTick() - last_command_tick : 0xFFFFFFFFU};
  uint8_t index = serial_port.tx_tail;
  uint8_t head = serial_port.tx_head;
  while (index != head)
  {
    ++snapshot.pending_frames;
    snapshot.pending_bytes = (uint16_t)(snapshot.pending_bytes + serial_port.tx_length[index]);
    index = (uint8_t)((index + 1U) % SERIAL_TX_QUEUE_DEPTH);
  }
  return snapshot;
}

uint8_t SerialDma_IsReady(void)
{
  return serial_port.ready;
}

SerialDmaSnapshot SerialDma_GetSnapshot(void)
{
  SerialDmaSnapshot snapshot = serial_stats;
  return snapshot;
}

SerialUartSnapshot SerialDma_GetUartSnapshot(void)
{
  SerialUartSnapshot snapshot = uart_stats;
  snapshot.baud = huart2.Init.BaudRate;
  snapshot.pclk_hz = HAL_RCC_GetPCLK1Freq();
  snapshot.brr = huart2.Instance->BRR;
  return snapshot;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart != &huart2)
  {
    return;
  }
  if (size > SERIAL_RX_DMA_SIZE)
  {
    size = SERIAL_RX_DMA_SIZE;
  }
  serial_stats.rx_bytes += size;
  /* IDLE ends a DMA burst, NOT a command. BLE may split even a short line.
   * Assemble bursts in the main loop until an explicit CR/LF is received. */
  SerialDma_PushRx(serial_port.rx_dma_buffer, size);
  if (SerialDma_StartRx() == 0U)
  {
    serial_port.rx_restart_needed = 1U;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((huart != &huart2) || (serial_port.tx_busy == 0U))
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
    ++serial_stats.uart_errors;
    uart_stats.last_error = huart->ErrorCode;
    if ((huart->ErrorCode & HAL_UART_ERROR_PE) != 0U) ++uart_stats.parity_errors;
    if ((huart->ErrorCode & HAL_UART_ERROR_NE) != 0U) ++uart_stats.noise_errors;
    if ((huart->ErrorCode & HAL_UART_ERROR_FE) != 0U) ++uart_stats.framing_errors;
    if ((huart->ErrorCode & HAL_UART_ERROR_ORE) != 0U) ++uart_stats.overrun_errors;
    if ((huart->ErrorCode & HAL_UART_ERROR_DMA) != 0U) ++uart_stats.dma_errors;
    serial_port.rx_fault = 1U;
    serial_port.rx_restart_needed = 1U;
    serial_port.ready = 0U;
  }
}
