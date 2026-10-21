#ifndef TOF_BUS_H
#define TOF_BUS_H
#include <stdint.h>

typedef enum { TOF_BUS_BUSY, TOF_BUS_OK, TOF_BUS_ERROR } TofBusResult;
void TofBus_Init(void);
void TofBus_Recover(void);
uint8_t TofBus_Read(uint16_t reg, uint8_t *data, uint16_t count);
uint8_t TofBus_Write(uint16_t reg, const uint8_t *data, uint16_t count);
TofBusResult TofBus_Poll(void);
#endif
