#include <assert.h>
#include <stdio.h>
#include "servo_test.h"

static uint32_t now;
static unsigned process_period = 1U;
static int key, starts, stops, attempts, moving, fail, fail_step, in_sample;
static int8_t directions[128];
uint32_t HAL_GetTick(void) { return now; }
int HAL_GPIO_ReadPin(int port, int pin) { (void)port; (void)pin; return !key; }
uint8_t St3215_Init(void) { assert(!in_sample); return !fail; }
uint8_t St3215_Start(void) { assert(!in_sample); ++starts; return !fail; }
uint8_t St3215_Stop(void) { assert(!in_sample); ++stops; return !fail; }
uint8_t St3215_GetMoving(uint8_t *value)
{
  assert(!in_sample); *value = (uint8_t)moving; return !fail;
}
uint8_t St3215_Step(int8_t direction)
{
  assert(!in_sample && attempts < 128);
  directions[attempts++] = direction;
  return !fail && !fail_step;
}
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
  key = held; starts = stops = attempts = moving = fail = fail_step = 0;
  process_period = 1U;
  ServoTest_Init();
}
static void click(void) { key = 1; advance(20); key = 0; advance(20); }
int main(void)
{
  reset(0); advance(10); assert(starts == 1 && attempts == 1 && directions[0] == 1);
  advance(1445); assert(attempts == 2 && directions[1] == -1);
  advance(1455); assert(attempts == 3 && directions[2] == 1);
  click(); assert(ServoTest_GetStatus() == 'S' && stops == 1);
  advance(5000); assert(attempts == 3);
  click(); assert(ServoTest_GetStatus() == 'R' && starts == 2 && attempts == 4);
  assert(directions[3] == 1);
  /* A held button toggles once, not once per foreground iteration. */
  key = 1; advance(2000); assert(ServoTest_GetStatus() == 'S');
  int stopped_attempts = attempts; advance(3000); assert(attempts == stopped_attempts);
  key = 0; advance(20);
  /* Contact bounce cannot toggle run/pause. */
  reset(0); advance(20);
  for (int i = 0; i < 4; ++i) { key=1; advance(5); key=0; advance(5); }
  assert(ServoTest_GetSnapshot().presses == 0 && ServoTest_GetStatus() == 'R');
  /* Short presses remain captured during a blocked foreground. */
  for (unsigned phase = 0; phase < 80; ++phase)
  {
    reset(0); advance(2); process_period=80U; advance(phase);
    key=1; advance(15); key=0; advance(100);
    assert(ServoTest_GetSnapshot().presses == 1 && ServoTest_GetStatus() == 'S');
  }
  /* A failed move acknowledgement must not cause an automatic relative retry. */
  reset(0); fail_step=1; advance(2); assert(attempts == 1 && ServoTest_GetStatus() == 'E');
  fail_step=0; advance(2500); assert(attempts == 1 && ServoTest_GetStatus() == 'S');
  click(); assert(attempts == 2 && ServoTest_GetStatus() == 'R');
  /* Feedback loss stops; recovery stays paused until the user presses again. */
  reset(0); advance(2); fail=1; advance(1300); assert(ServoTest_GetStatus() == 'E');
  fail=0; advance(700); assert(ServoTest_GetStatus() == 'S' && attempts == 1);
  click(); assert(attempts == 2);
  /* Never reverse a servo that has not completed the previous leg. */
  reset(0); moving=1; advance(3100);
  assert(attempts == 1 && ServoTest_GetSnapshot().errors > 0 && stops > 0);
  /* An already-held boot key does not repeatedly stop/start the automatic run. */
  reset(1); advance(600); assert(attempts == 1 && ServoTest_GetSnapshot().presses == 0);
  key=0; advance(20); click(); assert(ServoTest_GetStatus() == 'S');
  now=0xFFFFFF00U; reset(0); advance(1500);
  assert(attempts == 2 && directions[0] == 1 && directions[1] == -1);
  puts("PASS: boot auto/+30,-30 alternation/pause,resume/held key/bounce/80 short-press phases");
  puts("PASS: no relative retry after lost ACK/fault stays paused/motion timeout/boot-held/tick-wrap/no ISR UART");
  return 0;
}
