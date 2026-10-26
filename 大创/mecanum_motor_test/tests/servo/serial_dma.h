#ifndef TEST_SERIAL_DMA_H
#define TEST_SERIAL_DMA_H
#include <stdint.h>
typedef void (*SerialLineCallback)(const char *line);
uint8_t SerialDma_InitBluetooth(SerialLineCallback callback);
uint32_t SerialDma_GetRxErrors(void);
void SerialDma_Process(void);
uint8_t SerialDma_Write(const char *text);
#endif
