#include "servo_test.h"
#include "st3215.h"
#include "main.h"

#define DEBOUNCE_MS 10U
#define DOUBLE_MS 300U
#define HOLD_MS 500U
#define EVENT_COUNT 16U

typedef enum { STEP_FORWARD = 1, STEP_REVERSE, RUN, STOP } KeyEvent;

/* Single producer (SysTick), single consumer (main). Publish each event before
 * advancing head; release its slot before starting a UART operation. */
static volatile uint8_t events[EVENT_COUNT];
static volatile uint8_t head, tail, overflow, input_enabled, sample_holding;
static uint32_t edge_tick, press_tick, release_tick;
static uint8_t raw_key, stable_key, pending, second_press, released;
static uint32_t retry_tick, check_tick;
static uint8_t ready, running;

static void PushEvent(KeyEvent event)
{
  uint8_t next = (head + 1U) % EVENT_COUNT;
  if (next == tail) { overflow = 1U; return; }
  events[head] = (uint8_t)event;
  head = next;
}

static void Press(uint32_t now)
{
  press_tick = now;
  second_press = pending && ((now - release_tick) < DOUBLE_MS);
  if (pending && !second_press)
  {
    PushEvent(STEP_FORWARD);
    pending = 0U;
  }
}

static void Release(uint32_t now)
{
  if (!released) { released = 1U; return; }
  if (sample_holding)
  {
    sample_holding = 0U;
    PushEvent(STOP);
    return;
  }
  if (second_press)
  {
    pending = second_press = 0U;
    PushEvent(STEP_REVERSE);
    return;
  }
  pending = 1U;
  release_tick = now;
}

void ServoTest_Sample1ms(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t key = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
  if (!input_enabled)
  {
    raw_key = stable_key = key;
    edge_tick = press_tick = now;
    pending = second_press = sample_holding = 0U;
    released = !key;
    return;
  }
  if (key != raw_key) { raw_key = key; edge_tick = now; }
  if ((key != stable_key) && ((now - edge_tick) >= DEBOUNCE_MS))
  {
    stable_key = key;
    if (key) Press(now);
    else Release(now);
  }
  if (!released) return;
  if (stable_key && key && !sample_holding && ((now - press_tick) >= HOLD_MS))
  {
    pending = second_press = 0U;
    sample_holding = 1U;
    PushEvent(RUN);
  }
  if (pending && !stable_key && !key && ((now - release_tick) >= DOUBLE_MS))
  {
    pending = 0U;
    PushEvent(STEP_FORWARD);
  }
}

static void DisableInput(void)
{
  input_enabled = 0U;
  tail = head;
  overflow = 0U;
  running = 0U;
}

static void EnableInput(void)
{
  /* SysTick sees disabled input until its state is reset. */
  ServoTest_Sample1ms();
  input_enabled = 1U;
}

void ServoTest_Init(void)
{
  DisableInput();
  ready = St3215_Init();
  retry_tick = check_tick = HAL_GetTick();
  if (ready) EnableInput();
}

static void CheckResult(uint8_t ok)
{
  if (ok) return;
  ready = 0U;
  DisableInput();
  (void)St3215_Stop();
  retry_tick = HAL_GetTick();
}

void ServoTest_Process(void)
{
  uint32_t now = HAL_GetTick();
  KeyEvent event;
  if (overflow) CheckResult(0U);
  if (!ready)
  {
    if ((now - retry_tick) < 100U) return;
    ready = St3215_Init();
    retry_tick = HAL_GetTick();
    if (ready) EnableInput();
    return;
  }
  if (tail != head)
  {
    event = (KeyEvent)events[tail];
    tail = (tail + 1U) % EVENT_COUNT;
    switch (event)
    {
      case STEP_FORWARD: CheckResult(St3215_Step(1)); break;
      case STEP_REVERSE: CheckResult(St3215_Step(-1)); break;
      case RUN:
        /* Never start a delayed hold after the user has already released. */
        if (!sample_holding) break;
        running = 1U;
        check_tick = now;
        CheckResult(St3215_Run());
        break;
      case STOP:
        running = 0U;
        CheckResult(St3215_Stop());
        break;
      default: CheckResult(0U); break;
    }
    return;
  }
  if (running && ((now - check_tick) >= 100U))
  {
    check_tick = now;
    CheckResult(St3215_Check());
  }
}

char ServoTest_GetStatus(void)
{
  return ready ? (running ? 'R' : 'S') : 'E';
}
