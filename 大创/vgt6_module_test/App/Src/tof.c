#include "tof.h"
#include "tof_bus.h"
#include "stm32f4xx_hal.h"
#include "vl53l1x_defaults.h"
#include <string.h>

/* DL1B/VL53L1X, PB8/PB9 only. XSHUT is not wired to the MCU.
 * Long distance mode, 100 ms timing budget, 120 ms intermeasurement.
 * Sequence follows ST's ULD SensorInit calibration without its polling loops. */
typedef enum
{
  RESET_LOW, RESET_HIGH, BOOT, IDENTIFY, CONFIG,
  CAL_START, CAL_POLL, CAL_CLEAR, CAL_STOP, VHV_BOUND, VHV_START,
  TIMING_A, TIMING_B, OSCILLATOR, PERIOD, START,
  POLL, RESULT, CLEAR, RETRY
} TofPhase;
static TofSnapshot state;
static TofPhase phase;
static uint8_t pending, config_offset, transfer_count;
static uint8_t rx[16], tx[4];
static uint32_t due_tick, deadline_tick;

static uint8_t expired(uint32_t now, uint32_t deadline)
{
  return (int32_t)(now - deadline) >= 0;
}

static void fail(uint32_t now)
{
  pending = 0U;
  state.connected = state.ready = state.valid = 0U;
  state.distance_mm = 0U;
  state.range_status = 255U;
  ++state.errors;
  phase = RETRY;
  due_tick = now + 1000U;
}

void Tof_Init(void)
{
  memset(&state, 0, sizeof(state));
  state.range_status = 255U;
  pending = config_offset = 0U;
  phase = RESET_LOW;
  due_tick = HAL_GetTick() + 20U;
  TofBus_Init();
}

static void completed(uint32_t now)
{
  switch (phase)
  {
    case RESET_LOW: phase = RESET_HIGH; due_tick = now + 2U; break;
    case RESET_HIGH:
      phase = BOOT; due_tick = now + 5U; deadline_tick = now + 1000U;
      break;
    case BOOT:
      if (rx[0] & 1U) phase = IDENTIFY;
      else if (expired(now, deadline_tick)) fail(now);
      else due_tick = now + 5U;
      break;
    case IDENTIFY:
      state.model_id = (uint16_t)((rx[0] << 8) | rx[1]);
      if (state.model_id != 0xEACCU) { fail(now); break; }
      state.connected = 1U;
      config_offset = 0U;
      phase = CONFIG;
      break;
    case CONFIG:
      config_offset = (uint8_t)(config_offset + transfer_count);
      if (config_offset == sizeof(vl53l1x_defaults)) phase = CAL_START;
      break;
    case CAL_START:
      phase = CAL_POLL; due_tick = now + 10U; deadline_tick = now + 1000U;
      break;
    case CAL_POLL:
      /* GPIO_HV_MUX_CTRL=0x01: ready is active high, bit 0 of 0x0031. */
      if (rx[0] & 1U) phase = CAL_CLEAR;
      else if (expired(now, deadline_tick)) fail(now);
      else due_tick = now + 10U;
      break;
    case CAL_CLEAR: phase = CAL_STOP; break;
    case CAL_STOP: phase = VHV_BOUND; break;
    case VHV_BOUND: phase = VHV_START; break;
    case VHV_START: phase = TIMING_A; break;
    case TIMING_A: phase = TIMING_B; break;
    case TIMING_B: phase = OSCILLATOR; break;
    case OSCILLATOR:
    {
      uint32_t clock = ((uint16_t)(rx[0] << 8) | rx[1]) & 0x3FFU;
      uint32_t period = clock * 120U * 1075U / 1000U;
      if (!clock) { fail(now); break; }
      tx[0] = (uint8_t)(period >> 24); tx[1] = (uint8_t)(period >> 16);
      tx[2] = (uint8_t)(period >> 8); tx[3] = (uint8_t)period;
      phase = PERIOD;
      break;
    }
    case PERIOD: phase = START; break;
    case START:
      state.ready = 1U;
      phase = POLL; due_tick = now + 10U; deadline_tick = now + 600U;
      break;
    case POLL:
      if (rx[0] & 1U) phase = RESULT;
      else if (expired(now, deadline_tick)) fail(now);
      else due_tick = now + 10U;
      break;
    case RESULT:
      state.range_status = rx[0] & 0x1FU;
      state.distance_mm = (uint16_t)((rx[13] << 8) | rx[14]);
      state.valid = state.range_status == 9U && state.distance_mm > 0U &&
                    state.distance_mm <= 4000U;
      state.sample_tick = now;
      ++state.samples;
      phase = CLEAR;
      break;
    case CLEAR:
      phase = POLL; due_tick = now + 10U; deadline_tick = now + 600U;
      break;
    default: fail(now); break;
  }
}

static uint8_t write_byte(uint16_t reg, uint8_t value)
{
  return TofBus_Write(reg, &value, 1U); /* Bus owns a copy until completion. */
}

static uint8_t launch(void)
{
  static const uint8_t budget_a[] = {0x01U, 0xCCU};
  static const uint8_t budget_b[] = {0x01U, 0xEAU};
  switch (phase)
  {
    case RESET_LOW: return write_byte(0x0000U, 0U);
    case RESET_HIGH: return write_byte(0x0000U, 1U);
    case BOOT: return TofBus_Read(0x00E5U, rx, 1U);
    case IDENTIFY: return TofBus_Read(0x010FU, rx, 2U);
    case CONFIG:
      transfer_count = (uint8_t)(sizeof(vl53l1x_defaults) - config_offset);
      if (transfer_count > 16U) transfer_count = 16U;
      return TofBus_Write((uint16_t)(0x002DU + config_offset),
                          &vl53l1x_defaults[config_offset], transfer_count);
    case CAL_START: case START: return write_byte(0x0087U, 0x40U);
    case CAL_POLL: case POLL: return TofBus_Read(0x0031U, rx, 1U);
    case CAL_CLEAR: case CLEAR: return write_byte(0x0086U, 1U);
    case CAL_STOP: return write_byte(0x0087U, 0U);
    case VHV_BOUND: return write_byte(0x0008U, 9U);
    case VHV_START: return write_byte(0x000BU, 0U);
    case TIMING_A: return TofBus_Write(0x005EU, budget_a, 2U);
    case TIMING_B: return TofBus_Write(0x0061U, budget_b, 2U);
    case OSCILLATOR: return TofBus_Read(0x00DEU, rx, 2U);
    case PERIOD: return TofBus_Write(0x006CU, tx, 4U);
    case RESULT: return TofBus_Read(0x0089U, rx, 15U);
    default: return 0U;
  }
}

void Tof_Process(void)
{
  uint32_t now = HAL_GetTick();
  if (pending)
  {
    TofBusResult result = TofBus_Poll();
    if (result == TOF_BUS_BUSY) return;
    pending = 0U;
    if (result != TOF_BUS_OK) { fail(now); return; }
    completed(now);
    return;
  }
  if (!expired(now, due_tick)) return;
  if (phase == RETRY)
  {
    TofBus_Recover();
    phase = RESET_LOW;
  }
  if (!launch()) { fail(now); return; }
  pending = 1U;
}

TofSnapshot Tof_GetSnapshot(void)
{
  TofSnapshot copy = state;
  copy.age_ms = copy.samples ? HAL_GetTick() - copy.sample_tick : 0xFFFFFFFFU;
  if (!copy.ready || copy.age_ms > 350U) copy.valid = 0U;
  if (!copy.valid) copy.distance_mm = 0U;
  return copy;
}
