#ifndef TIMEBASE_H
#define TIMEBASE_H

#include "stm32f4xx_hal.h"

/* Main-loop capture using the existing 1 kHz HAL SysTick. No timer or
 * control scheduling changes. Both counters wrap as unsigned 32-bit values. */
static __inline void Timebase_Capture(uint32_t *tick_ms, uint32_t *tick_us)
{
  uint32_t mask = __get_PRIMASK();
  uint32_t ms, value, period;
  __disable_irq();
  ms = HAL_GetTick();
  period = SysTick->LOAD + 1U;
  value = SysTick->VAL;
  /* A rollover whose ISR has not run belongs to the next millisecond.
   * Re-read VAL after observing pending, never read CTRL/clear COUNTFLAG. */
  if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0U)
  {
    ++ms;
    value = SysTick->VAL;
  }
  __set_PRIMASK(mask);
  *tick_ms = ms;
  *tick_us = ms * 1000U + ((period - 1U - value) * 1000U) / period;
}

#endif
