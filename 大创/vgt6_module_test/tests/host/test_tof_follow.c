#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tof_follow.h"

static TofSnapshot range;
static uint32_t now;
static float yaw, rate;

static void step(uint16_t mm, uint8_t valid)
{
  now += 40U;
  range.sample_tick = now;
  range.age_ms = 0U;
  ++range.samples;
  range.distance_mm = mm;
  range.valid = valid;
  TofFollow_Process(&range, yaw, rate, now);
  assert(abs(TofFollow_GetSnapshot().forward_rpm) <= 80);
  assert(fabsf(TofFollow_GetSnapshot().turn_dps) <= 15.0f);
}

static void setup(uint16_t mm, float heading)
{
  memset(&range, 0, sizeof(range));
  range.connected = range.ready = range.valid = 1U;
  range.distance_mm = mm;
  range.samples = 1U;
  now = 100U;
  yaw = heading;
  rate = 0.0f;
  assert(TofFollow_Start(&range, yaw, now));
}

static void acquire(uint16_t mm)
{
  step(mm, 1); step(mm, 1); step(mm, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_TRACK);
}

static void test_control_and_filter(void)
{
  setup(1600, 0);
  step(1600, 1);
  for (unsigned i = 0; i < 20; ++i)
    TofFollow_Process(&range, yaw, rate, now); /* Same sample is not evidence. */
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_SEARCH);
  step(1600, 1); step(1600, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_TRACK);
  for (unsigned i = 0; i < 25; ++i) step(1600, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 33);
  /* A distant-background spike brakes immediately, with no filtered chase. */
  step(2400, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);
  step(1600, 1);
  assert(TofFollow_GetSnapshot().filtered_mm == 1600);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_TRACK);
  /* Median rejects one plausible in-band impulse. */
  step(1850, 1); step(1600, 1);
  assert(TofFollow_GetSnapshot().filtered_mm == 1600);

  setup(2000, 0); acquire(2000);
  for (unsigned i = 0; i < 40; ++i) step(2000, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 57); /* P gain unchanged. */
  setup(101, 0); acquire(101);
  for (unsigned i = 0; i < 40; ++i) step(101, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == -51);

  setup(600, 0); acquire(600);
  for (unsigned i = 0; i < 25; ++i) step(600, 1);
  assert(TofFollow_GetSnapshot().forward_rpm < 0);
  setup(1000, 0); acquire(1000);
  for (unsigned i = 0; i < 25; ++i) step(i % 2 ? 980 : 1020, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);
  setup(1600, 0); acquire(1600);
  rate = 12.0f;
  for (unsigned i = 0; i < 25; ++i) step(1600, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);
}

static void test_loss_search_and_confirmation(void)
{
  setup(1200, 179); acquire(1200);
  for (unsigned i = 0; i < 25; ++i) step(1200, 1);
  for (unsigned i = 0; i < 4; ++i) step(0, 0);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_SEARCH);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);
  assert(TofFollow_GetSnapshot().turn_dps > 0);
  yaw = -161; /* +20 degrees across wrap. */
  step(0, 0);
  assert(TofFollow_GetSnapshot().turn_dps < 0);
  yaw = 159;
  step(0, 0);
  assert(TofFollow_GetSnapshot().turn_dps > 0);
  step(1250, 1);
  assert(TofFollow_GetSnapshot().turn_dps == 0);
  step(1500, 1); /* Inconsistent second return restarts confirmation. */
  step(1500, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_SEARCH);
  step(1500, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_TRACK);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);

  setup(3500, 0);
  for (unsigned i = 0; i < 199; ++i)
    step(i % 4 == 0 ? 1000 : 3500, 1); /* Flicker cannot extend the deadline. */
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_SEARCH);
  step(3500, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_OFF);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_TIMEOUT);
}

static void test_fail_safe(void)
{
  setup(1600, 0); acquire(1600);
  for (unsigned i = 0; i < 25; ++i) step(1600, 1);
  range.age_ms = TOF_MAX_AGE_MS + 1;
  TofFollow_Process(&range, yaw, rate, now + 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);
  step(100, 1); /* Raw stop is immediate, despite the old 1600 mm filter. */
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_OFF);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_TOO_CLOSE);
  setup(1000, 0); range.ready = 0;
  step(1000, 1);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_SENSOR_FAULT);
  setup(3500, 0); yaw = 33;
  step(3500, 1);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_CONTROL_FAULT);
  setup(1000, 0); yaw = NAN;
  step(1000, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_OFF);
  setup(1000, 0);
  TofFollow_Process(&range, yaw, rate, now + 201);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_CONTROL_FAULT);
  setup(3500, 0);
  now = UINT32_MAX - 200;
  assert(TofFollow_Start(&range, yaw, now));
  for (unsigned i = 0; i < 200; ++i) step(3500, 1);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_TIMEOUT);
  TofFollow_Stop(TOF_FOLLOW_CANCELLED);
  step(1600, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_OFF);
}

static void test_synthetic_distance_plant(void)
{
  /* Assumed kinematic plant, not measured tuning/acceptance evidence. */
  const float initial[] = {600.0f, 1600.0f};
  for (unsigned j = 0; j < 2; ++j)
  {
    float distance = initial[j];
    setup((uint16_t)distance, 0);
    for (unsigned i = 0; i < 1250; ++i)
    {
      step((uint16_t)lroundf(distance), 1);
      distance -= 3.4f * TofFollow_GetSnapshot().forward_rpm * 0.04f;
      assert(TofFollow_GetSnapshot().mode != TOF_FOLLOW_OFF);
      assert(distance > 500 && distance < 1700);
    }
    assert(fabsf(distance - 1000.0f) < 65.0f);
    assert(abs(TofFollow_GetSnapshot().forward_rpm) <= 1);
  }
}

int main(void)
{
  test_control_and_filter();
  test_loss_search_and_confirmation();
  test_fail_safe();
  test_synthetic_distance_plant();
  puts("PASS: ToF follow range signs/limits/filter/deadband, no duplicate evidence, loss/reacquire, bounded search/timeout, raw stop/fault/wrap, assumed distance plant (not measured)");
  return 0;
}
