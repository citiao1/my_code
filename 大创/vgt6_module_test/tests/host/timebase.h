#ifndef HOST_TIMEBASE_H
#define HOST_TIMEBASE_H
#include "stm32f4xx_hal.h"
static inline void Timebase_Capture(uint32_t *ms, uint32_t *us)
{
  *ms = HAL_GetTick();
  *us = *ms * 1000U;
}
#endif
