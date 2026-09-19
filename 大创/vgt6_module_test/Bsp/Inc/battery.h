#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

typedef struct
{
  uint16_t raw_adc;
  uint16_t voltage_mv;
  uint16_t read_failures;
  uint8_t ready;
} BatterySnapshot;

void Battery_Init(void);
void Battery_Process(void);
BatterySnapshot Battery_GetSnapshot(void);

#endif /* BATTERY_H */
