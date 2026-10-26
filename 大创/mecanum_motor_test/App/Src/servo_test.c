#include "servo_test.h"
#include "st3215.h"
#include "main.h"

#define DEBOUNCE_MS 20U
#define DOUBLE_MS 300U
#define HOLD_MS 500U

static uint32_t edge_tick, press_tick, release_tick, retry_tick;
static uint8_t raw_key, stable_key, pending, holding, ready, stop_pending, released;

void ServoTest_Init(void)
{
  raw_key = stable_key = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
  released = !raw_key; /* Require release after boot before accepting a press. */
  pending = holding = stop_pending = 0U;
  edge_tick = retry_tick = HAL_GetTick();
  ready = St3215_Init();
}

static void CheckResult(uint8_t ok)
{
  ready = ok;
  if (!ok) { stop_pending = 1U; pending = 0U; }
}

void ServoTest_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t key = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
  if (key != raw_key) { raw_key = key; edge_tick = now; }
  if (stop_pending || !ready)
  {
    if ((now - retry_tick) < 100U) return;
    retry_tick = now;
    ready = St3215_Init();
    stop_pending = !ready;
    holding = pending = 0U;
    released = 0U;
    return;
  }
  if ((key != stable_key) && ((now - edge_tick) >= DEBOUNCE_MS))
  {
    stable_key = key;
    if (key) { press_tick = now; }
    else if (!released) { released = 1U; }
    else if (holding)
    {
      holding = 0U;
      CheckResult(St3215_Stop());
    }
    else if (pending && ((now - release_tick) < DOUBLE_MS))
    {
      pending = 0U;
      CheckResult(St3215_Step(-1));
    }
    else { pending = 1U; release_tick = now; }
  }
  if (!stable_key && !key && !released) released = 1U;
  if (!released) return;
  if (holding && ((now - retry_tick) >= 100U))
  {
    retry_tick = now;
    if (!St3215_Check())
    {
      CheckResult(0U);
      (void)St3215_Stop();
      return;
    }
  }
  if (stable_key && key && !holding && ((now - press_tick) >= HOLD_MS))
  {
    pending = 0U;
    holding = 1U;
    CheckResult(St3215_Run());
  }
  if (pending && !stable_key && !key && ((now - release_tick) >= DOUBLE_MS))
  {
    pending = 0U;
    CheckResult(St3215_Step(1));
  }
}

char ServoTest_GetStatus(void)
{
  return ready ? (holding ? 'R' : 'S') : 'E';
}
