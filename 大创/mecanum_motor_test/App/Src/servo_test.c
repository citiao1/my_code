#include "servo_test.h"
#include "st3215.h"
#include "main.h"

#define DEBOUNCE_MS 10U
#define LEG_MINIMUM_MS 1200U
#define LEG_TIMEOUT_MS 3000U
#define ENDPOINT_DWELL_MS 250U
#define FEEDBACK_PERIOD_MS 100U
#define RECOVERY_PERIOD_MS 500U

typedef enum { AUTO_IDLE, AUTO_MOVING, AUTO_DWELL } AutoPhase;

/* ISR owns only debounce and the monotonic press count. Main owns all motion,
 * request toggling and UART I/O, so a press remains visible during link faults. */
static volatile uint8_t input_initialized;
static volatile uint32_t presses;
static uint8_t raw_key, stable_key, released;
static uint32_t edge_tick;
static uint32_t handled_presses, commands, errors, leg_tick, poll_tick, retry_tick;
static uint8_t ready, requested, torque_active;
static int8_t direction;
static AutoPhase phase;
static char last_command;

void ServoTest_Sample1ms(void)
{
  uint32_t now;
  uint8_t key;
  if (!input_initialized) return;
  now = HAL_GetTick();
  key = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
  if (key != raw_key) { raw_key = key; edge_tick = now; }
  if ((key == stable_key) || ((now - edge_tick) < DEBOUNCE_MS)) return;
  stable_key = key;
  if (!key) { released = 1U; return; }
  if (released) { ++presses; released = 0U; }
}

static void Fault(void)
{
  ++errors;
  ready = requested = torque_active = 0U;
  phase = AUTO_IDLE;
  last_command = 'E';
  (void)St3215_Stop();
  retry_tick = HAL_GetTick();
}

static uint8_t Command(char name, uint8_t ok)
{
  last_command = name;
  if (!ok) { Fault(); return 0U; }
  ++commands;
  return 1U;
}

void ServoTest_Init(void)
{
  input_initialized = 0U;
  presses = handled_presses = commands = errors = 0U;
  raw_key = stable_key = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
  released = !raw_key;
  edge_tick = HAL_GetTick();
  input_initialized = 1U;
  requested = 1U; /* Explicitly requested automatic reciprocation on power-up. */
  torque_active = 0U;
  direction = 1;
  phase = AUTO_IDLE;
  last_command = '-';
  ready = St3215_Init();
  retry_tick = HAL_GetTick();
  if (!ready) Fault();
}

static void ProcessKey(void)
{
  uint32_t count = presses;
  uint32_t changed = count - handled_presses;
  handled_presses = count;
  if (changed & 1U) requested = !requested;
}

void ServoTest_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t moving;
  ProcessKey();
  if (!ready)
  {
    /* Idempotent torque-off/init only. A failed move is never sent again. */
    if ((now - retry_tick) < RECOVERY_PERIOD_MS) return;
    ready = St3215_Init();
    if (!ready) ++errors;
    retry_tick = HAL_GetTick();
    return;
  }
  if (!requested)
  {
    if (torque_active) (void)Command('S', St3215_Stop());
    torque_active = 0U;
    phase = AUTO_IDLE;
    direction = 1;
    return;
  }
  if (!torque_active)
  {
    if (!Command('R', St3215_Start())) return;
    torque_active = 1U;
    direction = 1;
    phase = AUTO_IDLE;
    return;
  }
  if (phase == AUTO_MOVING)
  {
    if ((now - leg_tick) >= LEG_TIMEOUT_MS) { Fault(); return; }
    if ((now - leg_tick) < LEG_MINIMUM_MS) return;
    if ((now - poll_tick) < FEEDBACK_PERIOD_MS) return;
    poll_tick = now;
    if (!St3215_GetMoving(&moving)) { Fault(); return; }
    if (moving) return;
    direction = -direction;
    phase = AUTO_DWELL;
    leg_tick = HAL_GetTick();
    return;
  }
  if ((phase == AUTO_DWELL) && ((now - leg_tick) < ENDPOINT_DWELL_MS)) return;
  if (!Command(direction > 0 ? '+' : '-', St3215_Step(direction))) return;
  leg_tick = poll_tick = HAL_GetTick();
  phase = AUTO_MOVING;
}

char ServoTest_GetStatus(void)
{
  return !ready ? 'E' : (requested ? 'R' : 'S');
}

ServoTestSnapshot ServoTest_GetSnapshot(void)
{
  ServoTestSnapshot snapshot;
  snapshot.presses = presses;
  snapshot.commands = commands;
  snapshot.errors = errors;
  snapshot.key_down = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
  snapshot.last_command = last_command;
  snapshot.status = ServoTest_GetStatus();
  return snapshot;
}
