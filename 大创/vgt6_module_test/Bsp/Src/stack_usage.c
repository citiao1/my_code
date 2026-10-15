#include "stack_usage.h"

#include <stddef.h>
#include "stm32f4xx_hal.h"

#define STACK_PAINT_PATTERN 0xA55A3CC3UL
#define STACK_PAINT_RESERVE_BYTES 256U
#define STACK_GUARD_WORDS 8U

/* The exported startup vector's first word is the initial MSP.
 * __initial_sp itself is only exported in the startup's MicroLIB branch. */
extern const uint32_t __Vectors[];
static volatile uint32_t *stack_bottom;
static uint8_t initialized;

void StackUsage_Init(void)
{
  uintptr_t top = (uintptr_t)__Vectors[0];
  uintptr_t active = (uintptr_t)__get_MSP();
  uintptr_t fill_end;
  uint32_t irq_mask;
  volatile uint32_t *word;

  initialized = 0U;
  stack_bottom = (volatile uint32_t *)(top - STACK_USAGE_BUDGET_BYTES);
  if ((active > top) || (active <= (uintptr_t)stack_bottom +
                                   STACK_PAINT_RESERVE_BYTES))
  {
    return;
  }
  /* Never paint the active caller frames. Startup IRQs also have room. */
  fill_end = (active - STACK_PAINT_RESERVE_BYTES) & ~(uintptr_t)3U;
  irq_mask = __get_PRIMASK();
  __disable_irq();
  for (word = stack_bottom; (uintptr_t)word < fill_end; ++word)
  {
    *word = STACK_PAINT_PATTERN;
  }
  initialized = 1U;
  __set_PRIMASK(irq_mask);
}

uint8_t StackUsage_IsHealthy(void)
{
  uint32_t index;

  if (initialized == 0U)
  {
    return 0U;
  }
  for (index = 0U; index < STACK_GUARD_WORDS; ++index)
  {
    if (stack_bottom[index] != STACK_PAINT_PATTERN)
    {
      return 0U;
    }
  }
  return 1U;
}

StackUsageSnapshot StackUsage_GetSnapshot(void)
{
  StackUsageSnapshot snapshot = {STACK_USAGE_BUDGET_BYTES, 0U, 0U, 0U};
  uint32_t unused_words = 0U;

  if (initialized == 0U)
  {
    return snapshot;
  }
  while ((unused_words < (STACK_USAGE_BUDGET_BYTES / 4U)) &&
         (stack_bottom[unused_words] == STACK_PAINT_PATTERN))
  {
    ++unused_words;
  }
  snapshot.free_bytes = unused_words * 4U;
  snapshot.used_bytes = STACK_USAGE_BUDGET_BYTES - snapshot.free_bytes;
  snapshot.guard_ok = StackUsage_IsHealthy();
  return snapshot;
}
