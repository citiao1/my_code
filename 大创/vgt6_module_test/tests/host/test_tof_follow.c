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
  assert(fabsf(TofFollow_GetSnapshot().turn_dps) <= 25.0f);
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
  for (unsigned i = 0; i < 50; ++i) step(1600, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == 69);
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
  assert(TofFollow_GetSnapshot().forward_rpm == 80); /* P gain unchanged; capped. */
  setup(101, 0); acquire(101);
  for (unsigned i = 0; i < 40; ++i) step(101, 1);
  assert(TofFollow_GetSnapshot().forward_rpm == -15);

  setup(200, 0); acquire(200);
  for (unsigned i = 0; i < 25; ++i) step(200, 1);
  assert(TofFollow_GetSnapshot().forward_rpm < 0);
  setup(400, 0); acquire(400);
  for (unsigned i = 0; i < 25; ++i) step(i % 2 ? 380 : 420, 1);
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
  step(0, 0); /* Loss is first observed at 179 degrees. */
  yaw = -175; /* Drift +6 degrees during the 120 ms loss confirmation. */
  for (unsigned i = 0; i < 3; ++i) step(0, 0);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_SEARCH);
  assert(TofFollow_GetSnapshot().forward_rpm == 0);
  assert(TofFollow_GetSnapshot().turn_dps > 0);
  yaw = -146; /* +35: keep turning (old +/-22 logic would stop/reverse). */
  step(0, 0);
  assert(TofFollow_GetSnapshot().turn_dps > 0);
  yaw = -136; /* +45 relative to FIRST loss, across wrap; reverse now. */
  step(0, 0);
  assert(TofFollow_GetSnapshot().turn_dps < 0);
  yaw = 134; /* -45 relative to the same fixed loss angle. */
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

  /* A new loss gets a new anchor; search never follows subsequent yaw. */
  yaw = 60;
  step(0, 0);
  yaw = 70;
  for (unsigned i = 0; i < 3; ++i) step(0, 0);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_SEARCH);
  yaw = 105;
  step(0, 0);
  assert(TofFollow_GetSnapshot().turn_dps < 0);

  setup(3500, 0);
  for (unsigned i = 0; i < 399; ++i)
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
  setup(3500, 0); yaw = 56;
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
  for (unsigned i = 0; i < 400; ++i) step(3500, 1);
  assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_TIMEOUT);
  TofFollow_Stop(TOF_FOLLOW_CANCELLED);
  step(1600, 1);
  assert(TofFollow_GetSnapshot().mode == TOF_FOLLOW_OFF);
}

static void test_synthetic_distance_plant(void)
{
  /* Assumed kinematic plant, not measured tuning/acceptance evidence. */
  const float initial[] = {200.0f, 1600.0f};
  for (unsigned j = 0; j < 2; ++j)
  {
    float distance = initial[j];
    setup((uint16_t)distance, 0);
    for (unsigned i = 0; i < 1250; ++i)
    {
      step((uint16_t)lroundf(distance), 1);
      distance -= 3.4f * TofFollow_GetSnapshot().forward_rpm * 0.04f;
      assert(TofFollow_GetSnapshot().mode != TOF_FOLLOW_OFF);
      assert(distance > 150 && distance < 1700);
    }
    assert(fabsf(distance - 400.0f) < 65.0f);
    assert(abs(TofFollow_GetSnapshot().forward_rpm) <= 1);
  }
}

static void test_synthetic_search_sweep(void)
{
  const float anchors[] = {0.0f, 179.0f};
  for (unsigned j = 0; j < 2; ++j)
  {
    float unwrapped = anchors[j], minimum = 0, maximum = 0;
    setup(3500, anchors[j]);
    for (unsigned i = 0; i < 400; ++i)
    {
      float offset;
      step(3500, 1);
      /* Assumed 150 ms yaw lag, not an identified vehicle model. */
      rate += (TofFollow_GetSnapshot().turn_dps - rate) * (0.04f / 0.15f);
      unwrapped += rate * 0.04f;
      yaw = unwrapped - 360.0f * floorf((unwrapped + 180.0f) / 360.0f);
      offset = unwrapped - anchors[j];
      if (offset < minimum) minimum = offset;
      if (offset > maximum) maximum = offset;
      assert(fabsf(offset) <= 45.0f);
    }
    assert(minimum < -44.0f && maximum > 44.0f);
    assert(TofFollow_GetSnapshot().reason == TOF_FOLLOW_TIMEOUT);
  }
}

int main(void)
{
  test_control_and_filter();
  test_loss_search_and_confirmation();
  test_fail_safe();
  test_synthetic_distance_plant();
  test_synthetic_search_sweep();
  puts("PASS: 400mm follow signs/80RPM limits/filter/deadband, fixed first-loss +/-45deg anchor/reacquire, 25dps/16s search, raw stop/fault/wrap, assumed distance/yaw plants (not measured)");
  return 0;
}
