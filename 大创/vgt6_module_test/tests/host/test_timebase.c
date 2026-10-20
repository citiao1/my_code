#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static struct { uint32_t LOAD, VAL; } systick;
static struct { uint32_t ICSR; } scb;
static uint32_t irq_mask, hal_tick;
#define SysTick (&systick)
#define SCB (&scb)
#define SCB_ICSR_PENDSTSET_Msk (1UL << 26)
#define __get_PRIMASK() irq_mask
#define __disable_irq() (irq_mask = 1U)
#define __set_PRIMASK(mask) (irq_mask = (mask))
uint32_t HAL_GetTick(void) { return hal_tick; }
#include "../../Bsp/Inc/timebase.h"

int main(void)
{
  uint32_t ms, us;
  systick.LOAD = 167999U;
  hal_tick = 123U;
  systick.VAL = 83999U;
  Timebase_Capture(&ms, &us);
  assert(ms == 123U && us == 123500U && irq_mask == 0U);
  systick.VAL = 0U;
  Timebase_Capture(&ms, &us);
  assert(us == 123999U);
  scb.ICSR = SCB_ICSR_PENDSTSET_Msk;
  systick.VAL = 167999U;
  irq_mask = 1U;
  Timebase_Capture(&ms, &us);
  assert(ms == 124U && us == 124000U && irq_mask == 1U);
  hal_tick = 0xffffffffU;
  Timebase_Capture(&ms, &us);
  assert(ms == 0U && us == 0U);
  scb.ICSR = 0U;
  hal_tick = 4294967U;
  systick.VAL = 0U;
  Timebase_Capture(&ms, &us);
  assert(us == 703U); /* Independent microsecond uint32 wrap. */
  puts("SysTick microsecond capture: phase, pending IRQ, mask and wraps passed");
  return 0;
}
