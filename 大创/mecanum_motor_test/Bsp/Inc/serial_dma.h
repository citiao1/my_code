#ifndef SERIAL_DMA_H
#define SERIAL_DMA_H

#include <stdint.h>

typedef void (*SerialLineCallback)(const char *line);

uint8_t SerialDma_Init(SerialLineCallback callback);
void SerialDma_Process(void);
uint8_t SerialDma_Write(const char *text);

#endif
