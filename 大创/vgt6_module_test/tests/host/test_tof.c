/* Real sensor state machine, emulated register bus. No physical hardware. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "tof.h"
#include "tof_bus.h"
#include "front_guard.h"

static uint32_t tick, last_measurement, calls;
static uint8_t memory[512], online, ranging, ready_enabled, next_status;
static uint16_t next_distance, pending_reg, pending_count;
static uint8_t *pending_rx;
static uint8_t pending_tx[16], writing, active;
uint32_t HAL_GetTick(void) { return tick; }
void TofBus_Init(void) { active = 0; }
void TofBus_Recover(void) { active = 0; }
uint8_t TofBus_Read(uint16_t reg, uint8_t *data, uint16_t count)
{
  ++calls;
  if (!online) return 0;
  assert(!active && count <= 16 && reg + count <= sizeof(memory));
  active = 1; writing = 0;
  pending_reg = reg; pending_count = count; pending_rx = data;
  return 1;
}
uint8_t TofBus_Write(uint16_t reg, const uint8_t *data, uint16_t count)
{
  ++calls;
  if (!online) return 0;
  assert(!active && count <= sizeof(pending_tx));
  active = 1; writing = 1;
  pending_reg = reg; pending_count = count;
  memcpy(pending_tx, data, count);
  return 1;
}
TofBusResult TofBus_Poll(void)
{
  assert(active);
  active = 0;
  if (!online) return TOF_BUS_ERROR;
  if (!writing) memcpy(pending_rx, memory + pending_reg, pending_count);
  else
  {
    memcpy(memory + pending_reg, pending_tx, pending_count);
    if (pending_reg == 0) memory[0xE5] = pending_tx[0] != 0;
    if (pending_reg == 0x87)
    {
      ranging = pending_tx[0] != 0;
      last_measurement = tick;
    }
    if (pending_reg == 0x86) memory[0x31] = 0;
  }
  return TOF_BUS_OK;
}
static void advance(uint32_t ms)
{
  for (uint32_t i = 0; i < ms; ++i)
  {
    ++tick;
    uint32_t period = ((uint32_t)memory[0x6C] << 24) |
        ((uint32_t)memory[0x6D] << 16) | ((uint32_t)memory[0x6E] << 8) | memory[0x6F];
    /* Default calibration uses 120 ms; user configuration uses OSC=512. */
    period = period == 512U * 40U * 1075U / 1000U ? 40U : 120U;
    if (ranging && ready_enabled && tick - last_measurement >= period)
    {
      memory[0x31] = 1;
      memory[0x89] = next_status;
      memory[0x96] = (uint8_t)(next_distance >> 8);
      memory[0x97] = (uint8_t)next_distance;
      last_measurement = tick;
    }
    Tof_Process();
  }
}
static void setup(uint32_t now)
{
  tick = now; calls = 0; active = ranging = 0;
  online = ready_enabled = 1;
  next_status = 0x89; next_distance = 1234;
  memset(memory, 0, sizeof(memory));
  memory[0x10F] = 0xEA; memory[0x110] = 0xCC;
  memory[0xDE] = 2; memory[0xDF] = 0; /* OSC=512 */
  Tof_Init();
}
int main(void)
{
  TofSnapshot s;
  setup(0);
  advance(650);
  s = Tof_GetSnapshot();
  assert(s.connected && s.ready && s.valid && s.distance_mm == 1234);
  assert(s.model_id == 0xEACC && s.samples && !s.errors);
  assert(memory[0x2E] == 1 && memory[0x2F] == 1);
  assert(memory[0x5E] == 0 && memory[0x5F] == 0x60);
  assert(memory[0x61] == 0 && memory[0x62] == 0x6E);
  assert(memory[0x6C] == 0 && memory[0x6D] == 0 && memory[0x6E] == 0x56 && memory[0x6F] == 0);
  /* Move a target at every phase of the 40 ms cycle. No stale repeated
   * samples may hide a <=100 mm obstacle beyond one measurement + bus reads. */
  for (uint32_t offset = 0; offset < 40; ++offset)
  {
    next_distance = 500; advance(50 + offset);
    assert(FrontGuard_ForwardLimit(160) == 160);
    uint32_t before_tick = tick;
    next_distance = 100;
    while (Tof_GetSnapshot().distance_mm != 100 && tick - before_tick <= 50) advance(1);
    assert(tick - before_tick <= 46);
    assert(FrontGuard_ForwardLimit(160) == 0);
  }
  /* Boundary, monotonicity and a lower global ceiling must all hold. */
  const uint16_t distances[] = {1, 99, 100, 101, 125, 150, 175, 199, 200, 201};
  const uint16_t limits[] = {0, 0, 0, 0, 10, 20, 30, 39, 40, 160};
  for (unsigned i = 0; i < sizeof(distances) / sizeof(distances[0]); ++i)
  {
    next_distance = distances[i]; advance(50);
    assert(FrontGuard_ForwardLimit(160) == limits[i]);
    assert(FrontGuard_ForwardLimit(15) == (limits[i] < 15 ? limits[i] : 15));
  }
  next_status = 4;
  advance(180);
  assert(!Tof_GetSnapshot().valid && !Tof_GetSnapshot().distance_mm);
  next_status = 0x89; next_distance = 8192;
  advance(180);
  assert(!Tof_GetSnapshot().valid);
  next_distance = 500;
  advance(180);
  assert(Tof_GetSnapshot().valid && Tof_GetSnapshot().distance_mm == 500);
  tick += TOF_MAX_AGE_MS + 1;
  assert(!Tof_GetSnapshot().valid && !Tof_GetSnapshot().distance_mm);
  online = 0;
  advance(50);
  s = Tof_GetSnapshot();
  assert(!s.connected && !s.valid && s.errors);
  uint32_t before = calls;
  advance(500);
  assert(calls == before); /* Failed sensor never spins or blocks the controller. */
  online = 1;
  advance(1600);
  assert(Tof_GetSnapshot().valid);
  setup(0xfffffeffU);
  advance(900);
  assert(Tof_GetSnapshot().valid); /* Scheduling and sample age survive tick wrap. */
  setup(0);
  memory[0x10F] = 0;
  advance(200);
  assert(!Tof_GetSnapshot().connected && Tof_GetSnapshot().errors);
  setup(0);
  ready_enabled = 0;
  advance(1300);
  assert(!Tof_GetSnapshot().ready && Tof_GetSnapshot().errors);
  puts("PASS: ToF 33/40 ms configuration, 40-phase near-stop latency <=46 ms, validity/staleness, unplug/retry and wrap");
  return 0;
}
