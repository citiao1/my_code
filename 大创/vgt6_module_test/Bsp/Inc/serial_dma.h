#ifndef SERIAL_DMA_H
#define SERIAL_DMA_H

#include <stdint.h>

typedef void (*SerialLineCallback)(const char *line);

typedef struct
{
  uint32_t rx_bytes;
  uint32_t rx_lines;
  uint32_t rx_overflows;
  uint32_t tx_dropped;
  uint32_t uart_errors;
} SerialDmaSnapshot;

typedef struct
{
  uint32_t baud;
  uint32_t pclk_hz;
  uint32_t brr;
  uint32_t last_error;
  uint32_t parity_errors;
  uint32_t noise_errors;
  uint32_t framing_errors;
  uint32_t overrun_errors;
  uint32_t dma_errors;
} SerialUartSnapshot;

typedef struct
{
  uint16_t pending_frames, pending_bytes;
  uint32_t telemetry_skipped, command_age_ms;
} SerialTxSnapshot;

uint8_t SerialDma_Init(SerialLineCallback callback);
void SerialDma_Process(void);
uint8_t SerialDma_Write(const char *text);
/* Optional current-state telemetry: never build a queue ahead of command replies. */
uint8_t SerialDma_WriteTelemetry(const char *text);
uint8_t SerialDma_CanWriteTelemetry(void);
SerialTxSnapshot SerialDma_GetTxSnapshot(void);
uint8_t SerialDma_IsReady(void);
SerialDmaSnapshot SerialDma_GetSnapshot(void);
SerialUartSnapshot SerialDma_GetUartSnapshot(void);

#endif /* SERIAL_DMA_H */
