#include "servo_test.h"
#include "st3215.h"
#include "main.h"

#define DEBOUNCE_MS 10U
#define ENDPOINT_DWELL_MS 250U
#define FEEDBACK_PERIOD_MS 100U
#define RECOVERY_PERIOD_MS 500U
#define REMOTE_LEASE_MS 3000U
#define CONTINUOUS_STALL_MS 1500U

typedef enum { AUTO_IDLE, AUTO_MOVING, AUTO_DWELL } AutoPhase;
typedef enum { MODE_AUTO, MODE_MOVE, MODE_CONTINUOUS } ControlMode;
static volatile uint8_t input_initialized;
static volatile uint32_t presses;
static uint8_t raw_key, stable_key, released;
static uint32_t edge_tick, handled_presses, commands, errors;
static uint32_t leg_tick, poll_tick, retry_tick, remote_tick;
static uint32_t rotation_tick;
static uint32_t leg_minimum_ms, leg_timeout_ms;
static uint8_t ready, requested, torque_active, remote_active;
static int8_t direction;
static int16_t angle_d10;
static uint16_t motion_speed;
static AutoPhase phase;
static ControlMode mode;
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
  ready = requested = torque_active = remote_active = 0U;
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
  requested = torque_active = remote_active = 0U; /* Bluetooth control starts in standby. */
  direction = 1;
  angle_d10 = 300;
  motion_speed = 500U;
  phase = AUTO_IDLE;
  mode = MODE_AUTO;
  last_command = '-';
  ready = St3215_Init();
  retry_tick = HAL_GetTick();
  if (!ready) Fault();
}

uint8_t ServoTest_Stop(void)
{
  requested = remote_active = torque_active = 0U;
  phase = AUTO_IDLE;
  direction = 1;
  return Command('S', St3215_Stop());
}

static uint8_t SetStepMode(int16_t angle, uint16_t speed, ControlMode selected)
{
  if (!ready || !angle || angle < -7200 || angle > 7200 || speed < 100U || speed > 1500U) return 0U;
  if (!Command('P', St3215_PrepareStep())) return 0U;
  angle_d10 = angle < 0 ? -angle : angle;
  direction = angle < 0 ? -1 : 1;
  motion_speed = speed;
  mode = selected;
  requested = remote_active = 1U;
  remote_tick = HAL_GetTick();
  torque_active = 0U;
  phase = AUTO_IDLE;
  return 1U;
}

uint8_t ServoTest_Move(int16_t angle, uint16_t speed) { return SetStepMode(angle, speed, MODE_MOVE); }
uint8_t ServoTest_Automatic(int16_t angle, uint16_t speed) { return SetStepMode(angle, speed, MODE_AUTO); }

uint8_t ServoTest_Continuous(int8_t rotation, uint16_t speed)
{
  if (!ready || (rotation != 1 && rotation != -1) || speed < 100U || speed > 1500U) return 0U;
  if (!Command('C', St3215_RunSpeed(rotation, speed))) return 0U;
  mode = MODE_CONTINUOUS;
  motion_speed = speed;
  direction = rotation;
  requested = torque_active = remote_active = 1U;
  phase = AUTO_IDLE;
  remote_tick = poll_tick = HAL_GetTick();
  rotation_tick = remote_tick;
  return 1U;
}

void ServoTest_KeepAlive(void) { remote_tick = HAL_GetTick(); }

static void ProcessKey(void)
{
  uint32_t count = presses;
  uint32_t changed = count - handled_presses;
  handled_presses = count;
  if (!(changed & 1U)) return;
  if (requested) { (void)ServoTest_Stop(); return; }
  if (!ready || !Command('P', St3215_PrepareStep())) return;
  mode = MODE_AUTO;
  requested = 1U;
  remote_active = torque_active = 0U;
  direction = 1;
  phase = AUTO_IDLE;
}

void ServoTest_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t counts;
  uint8_t moving;
  int16_t speed;
  ProcessKey();
  if (!ready)
  {
    if ((now - retry_tick) < RECOVERY_PERIOD_MS) return;
    ready = St3215_Init();
    if (!ready) ++errors;
    retry_tick = HAL_GetTick();
    return;
  }
  if (!requested) return;
  if (remote_active && ((now - remote_tick) >= REMOTE_LEASE_MS))
  {
    (void)ServoTest_Stop();
    last_command = 'T';
    return;
  }
  if (mode == MODE_CONTINUOUS)
  {
    if ((now - poll_tick) < FEEDBACK_PERIOD_MS) return;
    poll_tick = now;
    if (!St3215_GetSpeed(&speed)) { Fault(); return; }
    if ((direction > 0 && speed >= 10) || (direction < 0 && speed <= -10)) rotation_tick = now;
    else if ((now - rotation_tick) >= CONTINUOUS_STALL_MS) Fault();
    return;
  }
  if (!torque_active)
  {
    if (!Command('R', St3215_Start())) return;
    torque_active = 1U;
    phase = AUTO_IDLE;
    return;
  }
  if (phase == AUTO_MOVING)
  {
    if ((now - leg_tick) >= leg_timeout_ms) { Fault(); return; }
    if ((now - leg_tick) < leg_minimum_ms || (now - poll_tick) < FEEDBACK_PERIOD_MS) return;
    poll_tick = now;
    if (!St3215_GetMoving(&moving)) { Fault(); return; }
    if (moving) return;
    if (mode == MODE_MOVE) { (void)ServoTest_Stop(); return; }
    direction = -direction;
    phase = AUTO_DWELL;
    leg_tick = HAL_GetTick();
    return;
  }
  if (phase == AUTO_DWELL && ((now - leg_tick) < ENDPOINT_DWELL_MS)) return;
  if (!Command(direction > 0 ? '+' : '-', St3215_MoveAngle((int16_t)(direction * angle_d10), motion_speed))) return;
  counts = ((uint32_t)angle_d10 * 4096U + 1800U) / 3600U;
  leg_minimum_ms = counts * 1000U / motion_speed + 500U;
  leg_timeout_ms = leg_minimum_ms + 2000U;
  leg_tick = poll_tick = HAL_GetTick();
  phase = AUTO_MOVING;
}

char ServoTest_GetStatus(void) { return !ready ? 'E' : (requested ? 'R' : 'S'); }
const char *ServoTest_GetModeText(void)
{
  if (!ready) return "FAULT";
  if (!requested) return "PAUSE";
  return mode == MODE_CONTINUOUS ? "RUN" : (mode == MODE_MOVE ? "MOVE" : "AUTO");
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
