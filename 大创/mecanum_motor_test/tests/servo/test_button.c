#include <assert.h>
#include <stdio.h>
#include "servo_test.h"

static uint32_t now;
static int key, steps, runs, stops, fail, in_sample;
static unsigned process_period = 1U;
uint32_t HAL_GetTick(void) { return now; }
int HAL_GPIO_ReadPin(int port, int pin) { (void)port; (void)pin; return !key; }
uint8_t St3215_Init(void) { assert(!in_sample); return !fail; }
uint8_t St3215_Check(void) { assert(!in_sample); return !fail; }
uint8_t St3215_Stop(void) { assert(!in_sample); ++stops; return !fail; }
uint8_t St3215_Step(int8_t direction) { assert(!in_sample); steps += direction; return !fail; }
uint8_t St3215_Run(void) { assert(!in_sample); ++runs; return !fail; }
static void advance(unsigned ms)
{
  while (ms--)
  {
    ++now;
    in_sample = 1; ServoTest_Sample1ms(); in_sample = 0;
    if (process_period && now % process_period == 0) ServoTest_Process();
  }
}
static void reset(int held)
{
  key = held; steps = runs = stops = fail = 0;
  process_period = 1U;
  ServoTest_Init();
}
static void click(void) { key = 1; advance(70); key = 0; advance(70); }
int main(void)
{
  reset(0); click(); assert(steps == 0); advance(310); assert(steps == 1);
  reset(0); click(); advance(50); click(); advance(400); assert(steps == -1);
  reset(0); key = 1; advance(550); assert(runs == 1 && steps == 0);
  advance(1500); assert(runs == 1); key = 0; advance(30);
  assert(stops == 1); advance(500); assert(steps == 0);
  reset(0); key = 1; advance(5); key = 0; advance(5);
  key = 1; advance(5); key = 0; advance(600); assert(steps == 0 && runs == 0);
  reset(1); advance(900); assert(runs == 0); key = 0; advance(50);
  click(); advance(400); assert(steps == 1);
  reset(0); click(); key = 1; advance(550); assert(runs == 1 && steps == 0);
  key = 0; advance(30); assert(stops == 1);
  reset(0); now = 0xFFFFFF00U; click(); advance(400); assert(steps == 1);
  reset(0); fail = 1; click(); advance(400); assert(ServoTest_GetStatus() == 'E');
  fail = 0; advance(250); assert(ServoTest_GetStatus() == 'S');
  /* A complete short click can happen while the foreground is blocked. */
  for (unsigned phase = 0; phase < 80; ++phase)
  {
    reset(0); process_period = 80U; advance(phase);
    key = 1; advance(15); key = 0; advance(420);
    assert(steps == 1);
  }
  /* Second PRESS fits the window; second release is much later. */
  reset(0); click(); advance(180); key = 1; advance(200);
  key = 0; advance(400); assert(steps == -1);
  /* Hold/release fully captured during a stalled foreground must not start. */
  reset(0); process_period = 0U; key = 1; advance(650);
  key = 0; advance(50); process_period = 1U; advance(50);
  assert(runs == 0 && steps == 0 && stops == 1);
  /* Bounded queue overflow enters stop/recovery rather than dropping STOP. */
  reset(0); process_period = 0U;
  for (unsigned i = 0; i < 20; ++i) { click(); advance(350); }
  process_period = 1U; advance(1); assert(ServoTest_GetStatus() == 'E');
  advance(200); assert(steps == 0 && runs == 0 && stops > 0);
  puts("PASS: single/double/hold/release/bounce/boot-held/second-hold/tick-wrap/recovery");
  puts("PASS: 80 foreground phases/15ms presses/late second release/no stale RUN/overflow/no ISR UART");
  return 0;
}
