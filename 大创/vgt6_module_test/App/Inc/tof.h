#ifndef TOF_H
#define TOF_H
#include <stdint.h>

typedef struct
{
  uint32_t sample_tick, age_ms, errors, samples;
  uint16_t distance_mm, model_id;
  uint8_t connected, ready, valid, range_status;
} TofSnapshot;

void Tof_Init(void);
void Tof_Process(void);
TofSnapshot Tof_GetSnapshot(void);
#endif
