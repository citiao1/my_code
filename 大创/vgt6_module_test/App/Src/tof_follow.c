#include "tof_follow.h"

#include <math.h>
#include <string.h>
#include "front_guard.h"

/* Unmeasured conservative outer-loop defaults; the existing wheel and yaw
 * controllers are unchanged. Positive forward approaches the TOF end.
 * Single-beam range continuity is a heuristic, not target identification. */
#define FOLLOW_DEADBAND_MM 50.0f
#define FOLLOW_DEFAULT_FORWARD_P 0.06f
#define FOLLOW_DEFAULT_REVERSE_P 0.18f
#define FOLLOW_ACCEL_RPM_S 60.0f
#define FOLLOW_FILTER_MS 80.0f
#define FOLLOW_MAX_RANGE_MM 2000U
#define FOLLOW_JUMP_MM 400.0f
#define FOLLOW_CONFIRM_JUMP_MM 120
#define FOLLOW_CONFIRM_SAMPLES 3U
#define FOLLOW_LOST_MS 120U
#define FOLLOW_SEARCH_MS 16000U
#define FOLLOW_SEARCH_ANGLE_DEG 45.0f
#define FOLLOW_SEARCH_MAX_DPS 25.0f
#define FOLLOW_SEARCH_GUARD_DEG 55.0f
#define FOLLOW_REACQUIRE_MS 250U
#define FOLLOW_PERIOD_MS 20U

static struct
{
  TofFollowSnapshot output;
  uint32_t last_sequence, last_sample_tick, control_tick, filter_tick;
  uint32_t search_tick, lost_tick, acquired_tick;
  uint16_t window[3], candidate_mm;
  uint8_t have_sample, window_index, candidate_count, losing;
  int8_t search_direction;
  float filtered_mm, reference_rpm, search_anchor, lost_yaw;
} follow;
static TofFollowGains gains;
static uint16_t target_mm;

static float clamp(float x, float bound)
{
  return fmaxf(-bound, fminf(bound, x));
}

static float wrap(float x)
{
  return x - 360.0f * floorf((x + 180.0f) / 360.0f);
}

static uint16_t median(void)
{
  uint16_t a = follow.window[0], b = follow.window[1], c = follow.window[2];
  if (a > b) { uint16_t t = a; a = b; b = t; }
  if (b > c) b = c;
  return a > b ? a : b;
}

static uint8_t usable(const TofSnapshot *range)
{
  return range->connected && range->ready && range->valid &&
      range->samples && range->age_ms <= TOF_MAX_AGE_MS &&
      range->distance_mm > 0U &&
      range->distance_mm <= FOLLOW_MAX_RANGE_MM;
}

void TofFollow_Stop(TofFollowReason reason)
{
  memset(&follow, 0, sizeof(follow));
  follow.output.reason = reason;
}

void TofFollow_Init(void)
{
  gains.forward = FOLLOW_DEFAULT_FORWARD_P;
  gains.reverse = FOLLOW_DEFAULT_REVERSE_P;
  target_mm = TOF_FOLLOW_DEFAULT_TARGET_MM;
  TofFollow_Stop(TOF_FOLLOW_NORMAL);
}

TofFollowGains TofFollow_GetGains(void) { return gains; }
uint16_t TofFollow_GetTarget(void) { return target_mm; }

uint8_t TofFollow_SetTarget(uint32_t distance_mm)
{
  if (follow.output.mode || distance_mm < TOF_FOLLOW_MIN_TARGET_MM ||
      distance_mm > TOF_FOLLOW_MAX_TARGET_MM) return 0U;
  target_mm = (uint16_t)distance_mm;
  return 1U;
}

uint8_t TofFollow_SetGains(float forward, float reverse)
{
  if (follow.output.mode || !isfinite(forward) || !isfinite(reverse) ||
      forward < 0.0f || forward > 1.0f || reverse < 0.0f || reverse > 1.0f)
    return 0U;
  gains.forward = forward;
  gains.reverse = reverse;
  return 1U;
}

static void search(float yaw, uint32_t now)
{
  follow.output.mode = TOF_FOLLOW_SEARCH;
  follow.output.forward_rpm = 0;
  follow.output.turn_dps = 0.0f;
  follow.reference_rpm = 0.0f;
  follow.search_anchor = yaw;
  follow.search_direction = 1;
  follow.search_tick = now;
  follow.candidate_count = follow.losing = 0U;
}

uint8_t TofFollow_Start(const TofSnapshot *range, float yaw_deg, uint32_t now)
{
  TofFollow_Stop(TOF_FOLLOW_NORMAL); /* L1 must preserve accepted RAM gains. */
  if (!range->connected || !range->ready || !isfinite(yaw_deg))
  {
    follow.output.reason = TOF_FOLLOW_SENSOR_FAULT;
    return 0U;
  }
  follow.control_tick = now;
  search(yaw_deg, now);
  return 1U;
}

static void acquire(const TofSnapshot *range, uint32_t now)
{
  int difference = (int)range->distance_mm - (int)follow.candidate_mm;
  if (!follow.candidate_count || difference > FOLLOW_CONFIRM_JUMP_MM ||
      difference < -(int)FOLLOW_CONFIRM_JUMP_MM)
    follow.candidate_count = 0U;
  follow.candidate_mm = range->distance_mm;
  if (++follow.candidate_count < FOLLOW_CONFIRM_SAMPLES) return;
  follow.output.mode = TOF_FOLLOW_TRACK;
  follow.filtered_mm = (float)range->distance_mm;
  follow.output.filtered_mm = range->distance_mm;
  follow.window[0] = follow.window[1] = follow.window[2] = range->distance_mm;
  follow.window_index = follow.losing = 0U;
  follow.filter_tick = now;
  follow.acquired_tick = now;
  follow.reference_rpm = 0.0f;
  follow.output.turn_dps = 0.0f;
}

static void filter_range(const TofSnapshot *range, uint32_t now)
{
  float dt_ms = (float)(now - follow.filter_tick);
  follow.window[follow.window_index] = range->distance_mm;
  follow.window_index = (uint8_t)((follow.window_index + 1U) % 3U);
  follow.filtered_mm += dt_ms / (FOLLOW_FILTER_MS + dt_ms) *
      ((float)median() - follow.filtered_mm);
  follow.filter_tick = now;
  follow.output.filtered_mm = (uint16_t)lroundf(follow.filtered_mm);
}

static void update_search(float yaw, uint32_t now)
{
  float displacement = wrap(yaw - follow.search_anchor);
  float error;
  if (now - follow.search_tick >= FOLLOW_SEARCH_MS)
  {
    TofFollow_Stop(TOF_FOLLOW_TIMEOUT);
    return;
  }
  if (fabsf(displacement) > FOLLOW_SEARCH_GUARD_DEG)
  {
    TofFollow_Stop(TOF_FOLLOW_CONTROL_FAULT);
    return;
  }
  if (displacement >= FOLLOW_SEARCH_ANGLE_DEG - 0.5f)
    follow.search_direction = -1;
  else if (displacement <= -FOLLOW_SEARCH_ANGLE_DEG + 0.5f)
    follow.search_direction = 1;
  error = follow.search_direction * FOLLOW_SEARCH_ANGLE_DEG - displacement;
  /* Pause immediately while confirming a possible target; never translate
   * during search. Gyro feedback bounds the sweep, rather than timed spins. */
  follow.output.turn_dps = follow.candidate_count ? 0.0f :
      clamp(error * 0.8f, FOLLOW_SEARCH_MAX_DPS);
}

void TofFollow_Process(const TofSnapshot *range, float yaw_deg,
                       float rate_dps, uint32_t now)
{
  uint8_t fresh, valid;
  uint32_t elapsed;
  float error, target, step;
  if (follow.output.mode == TOF_FOLLOW_OFF) return;
  if (!isfinite(yaw_deg) || !isfinite(rate_dps))
  {
    TofFollow_Stop(TOF_FOLLOW_CONTROL_FAULT);
    return;
  }
  if (!range->connected || !range->ready)
  {
    TofFollow_Stop(TOF_FOLLOW_SENSOR_FAULT);
    return;
  }
  /* In follow mode valid close returns remain distance feedback: no 100 mm
   * lockout. The independent forward guard still prevents forward approach. */
  fresh = !follow.have_sample || range->samples != follow.last_sequence ||
      range->sample_tick != follow.last_sample_tick;
  follow.have_sample = 1U;
  follow.last_sequence = range->samples;
  follow.last_sample_tick = range->sample_tick;
  valid = usable(range);
  if (follow.output.mode == TOF_FOLLOW_TRACK && valid &&
      range->distance_mm > FRONT_GUARD_STOP_MM &&
      fabsf((float)range->distance_mm - follow.filtered_mm) > FOLLOW_JUMP_MM)
    valid = 0U;
  if (!valid)
  {
    follow.output.forward_rpm = 0;
    follow.reference_rpm = 0.0f;
    follow.candidate_count = 0U;
    if (!follow.losing)
    {
      /* Freeze the first-loss heading before debounce; do not recenter the
       * sweep on a later heading while the chassis is still turning. */
      follow.lost_tick = now;
      follow.lost_yaw = yaw_deg;
      follow.losing = 1U;
    }
    if (follow.output.mode == TOF_FOLLOW_TRACK &&
        now - follow.lost_tick >= FOLLOW_LOST_MS)
      search(follow.lost_yaw, now);
  }
  else if (fresh)
  {
    follow.losing = 0U;
    if (follow.output.mode == TOF_FOLLOW_SEARCH) acquire(range, now);
    else filter_range(range, now);
  }
  elapsed = now - follow.control_tick;
  if (elapsed > 200U)
  {
    TofFollow_Stop(TOF_FOLLOW_CONTROL_FAULT);
    return;
  }
  if (follow.output.mode == TOF_FOLLOW_SEARCH)
  {
    update_search(yaw_deg, now);
    if (elapsed >= FOLLOW_PERIOD_MS) follow.control_tick = now;
    return;
  }
  follow.output.turn_dps = 0.0f;
  if (elapsed < FOLLOW_PERIOD_MS) return;
  follow.control_tick = now;
  if (!valid || now - follow.acquired_tick < FOLLOW_REACQUIRE_MS ||
      fabsf(rate_dps) > 5.0f)
  {
    follow.output.forward_rpm = 0;
    follow.reference_rpm = 0.0f;
    return;
  }
  error = follow.filtered_mm - (float)target_mm;
  target = fabsf(error) <= FOLLOW_DEADBAND_MM ? 0.0f :
      clamp((error > 0.0f ? gains.forward : gains.reverse) *
            (error - copysignf(FOLLOW_DEADBAND_MM, error)), (float)TOF_FOLLOW_MAX_RPM);
  /* Deadband and sign reversal brake immediately; acceleration is bounded. */
  if (target == 0.0f || target * follow.reference_rpm < 0.0f)
    follow.reference_rpm = 0.0f;
  step = FOLLOW_ACCEL_RPM_S * (float)elapsed / 1000.0f;
  follow.reference_rpm += clamp(target - follow.reference_rpm, step);
  follow.output.forward_rpm = (int16_t)lroundf(follow.reference_rpm);
}

TofFollowSnapshot TofFollow_GetSnapshot(void) { return follow.output; }
