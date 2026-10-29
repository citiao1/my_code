#ifndef TOF_H
#define TOF_H
#include <stdint.h>

/* ST ULD long-mode profile; keep the existing range while reducing latency. */
#define TOF_BUDGET_MS 33U
#define TOF_PERIOD_MS 40U
#define TOF_POLL_MS 2U
#define TOF_MAX_AGE_MS 120U

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
