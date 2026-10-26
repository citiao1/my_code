#include <assert.h>
#include <stdio.h>
#include "servo_test.h"

static uint32_t now;
static int key, steps, runs, stops, fail;
uint32_t HAL_GetTick(void) { return now; }
int HAL_GPIO_ReadPin(int port, int pin) { (void)port; (void)pin; return !key; }
uint8_t St3215_Init(void) { return !fail; }
uint8_t St3215_Check(void) { return !fail; }
uint8_t St3215_Stop(void) { ++stops; return !fail; }
uint8_t St3215_Step(int8_t direction) { steps += direction; return !fail; }
uint8_t St3215_Run(void) { ++runs; return !fail; }
static void advance(unsigned ms)
{
  while (ms--) { ++now; ServoTest_Process(); }
}
static void reset(int held)
{
  key = held; steps = runs = stops = fail = 0;
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
  puts("PASS: single/double/hold/release/bounce/boot-held/second-hold/tick-wrap/recovery");
  return 0;
}
