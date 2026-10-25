#include "attitude_control.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "chassis_test.h"
#include "gyro.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"
#include "telemetry.h"
#include "timebase.h"

#define ATTITUDE_WATCHDOG_MS 1500U
/* High-level motion shaping; raw HEAD/YAW tuning keeps its controller form. */
#define MANUAL_RATE_ACCEL_DPS2 400.0f
#define MANUAL_BRAKE_SETTLE_MS 120U
#define MANUAL_BRAKE_TIMEOUT_MS 2500U
#define MANUAL_D_FILTER_SECONDS 0.04f
#define MANUAL_TRACK_ERROR_DEG 8.0f
#define MANUAL_HOLD_RATE_DPS 30.0f
#define MANUAL_OUTPUT_SLEW_RPM_S 240.0f
#define MANUAL_REST_ENTER_DEG 0.35f
#define MANUAL_REST_EXIT_DEG 0.8f
#define MANUAL_REST_ENTER_DPS 1.5f
#define MANUAL_REST_EXIT_DPS 3.0f
#define TRACK_TRANSLATION_ACCEL_RPM_S 100.0f
#define TRACK_MAX_TRANSLATION_RPM 100.0f
#define TRACK_REST_SETTLE_MS 120U
#define DEGREES_TO_RADIANS 0.017453292519943295f
static AttitudeControlSnapshot state;
static struct
{
  uint8_t active, resting, braking;
  uint32_t brake_ms, settled_ms;
  float requested_dps, reference_dps, damping_dps;
} manual;
static struct
{
  uint8_t active, resting;
  uint32_t settled_ms;
  float damping_dps;
  float field_forward, field_left, reference_forward, reference_left;
} track;
static float integral_output;
static int16_t forward_rpm, left_rpm;
static uint32_t command_tick, accumulated_ms;
static float clamp(float x, float limit)
{
  return fmaxf(-limit, fminf(limit, x));
}
static float wrap(float x)
{
  return x - 360.0f * floorf((x + 180.0f) / 360.0f);
}
void AttitudeControl_Init(void)
{
  memset(&state, 0, sizeof(state));
  /* Startup defaults only. YAWPID/HEADPID update these RAM fields at runtime;
   * Stop and the control loop preserve the latest accepted gains. */
  state.rate_kp = 0.1815f;
  state.rate_ki = 1.8062f;
  state.heading_kp = 5.5f;
  state.heading_kd = 2.5f;
  AttitudeControl_Stop();
}
void AttitudeControl_Stop(void)
{
  state.mode = 0U;
  state.saturated = 0U;
  state.target_dps = state.output_rpm = state.output_dps = 0.0f;
  integral_output = 0.0f;
  forward_rpm = left_rpm = 0;
  state.forward_rpm = 0;
  state.left_rpm = 0;
  accumulated_ms = 0U;
  memset(&manual, 0, sizeof(manual));
  memset(&track, 0, sizeof(track));
}
static uint8_t ready(void)
{
  GyroSnapshot gyro = Gyro_GetSnapshot();
  return gyro.ready && !gyro.read_failures &&
         ChassisTest_GetSnapshot().armed;
}
static uint8_t start(uint8_t mode)
{
  if (!ready()) return 0U;
  if (state.mode != mode)
  {
    ChassisTest_Stop();
    if (!SpeedControl_Enable(1U)) return 0U;
    state.mode = mode;
  }
  command_tick = HAL_GetTick();
  return 1U;
}
uint8_t AttitudeControl_SetRate(float rate_dps, int16_t forward, int16_t left)
{
  if (!isfinite(rate_dps) || fabsf(rate_dps) > ATTITUDE_RATE_LIMIT_DPS ||
      abs((int)forward) + abs((int)left) > SPEED_CONTROL_MAX_TARGET_RPM)
    return 0U;
  if (!start(1U)) return 0U;
  manual.active = 0U;
  track.active = 0U;
  state.target_dps = rate_dps;
  forward_rpm = forward;
  left_rpm = left;
  return 1U;
}
uint8_t AttitudeControl_SetHeading(float heading_deg)
{
  return AttitudeControl_SetHeadingWithDrive(heading_deg, 0, 0);
}

uint8_t AttitudeControl_SetManualDrive(float rate_dps,
                                      int16_t forward, int16_t left)
{
  GyroSnapshot gyro = Gyro_GetSnapshot();

  if (!isfinite(rate_dps) || fabsf(rate_dps) > ATTITUDE_RATE_LIMIT_DPS ||
      abs((int)forward) + abs((int)left) > SPEED_CONTROL_MAX_TARGET_RPM ||
      !isfinite(gyro.yaw_deg) || !isfinite(gyro.rate_dps) || !ready() ||
      !SpeedControl_GetSnapshot().enabled)
    return 0U;
  if (!state.mode && !start(2U)) return 0U;
  if (!manual.active)
  {
    track.active = 0U;
    memset(&manual, 0, sizeof(manual));
    manual.active = 1U;
    manual.damping_dps = gyro.rate_dps;
    state.target_deg = wrap(gyro.yaw_deg);
    integral_output = 0.0f;
  }
  /* Release cancels angle tracking immediately. Brake actual rate first;
   * do not choose a held angle until the measured rotation has settled. */
  if (manual.requested_dps != 0.0f && rate_dps == 0.0f)
  {
    manual.braking = 1U;
    manual.resting = 0U;
    manual.brake_ms = manual.settled_ms = 0U;
    manual.reference_dps = 0.0f;
    integral_output = 0.0f;
  }
  if (manual.braking && rate_dps != 0.0f)
  {
    manual.braking = 0U;
    manual.reference_dps = clamp(gyro.rate_dps, ATTITUDE_RATE_LIMIT_DPS);
    state.target_deg = wrap(gyro.yaw_deg);
  }
  manual.requested_dps = rate_dps;
  if (rate_dps != 0.0f) manual.resting = 0U;
  state.mode = 2U;
  forward_rpm = forward;
  left_rpm = left;
  command_tick = HAL_GetTick();
  return 1U;
}

uint8_t AttitudeControl_SetHeadingWithDrive(float heading_deg,
                                            int16_t forward,
                                            int16_t left)
{
  if (!isfinite(heading_deg) || fabsf(heading_deg) > 180.0f) return 0U;
  if (abs((int)forward) + abs((int)left) >
      SPEED_CONTROL_MAX_TARGET_RPM)
    return 0U;
  if (!start(2U)) return 0U;
  manual.active = 0U;
  track.active = 0U;
  state.target_deg = wrap(heading_deg);
  forward_rpm = forward;
  left_rpm = left;
  return 1U;
}
uint8_t AttitudeControl_SetTrackedDrive(float heading_deg,
                                        float field_forward, float field_left)
{
  GyroSnapshot gyro = Gyro_GetSnapshot();
  uint8_t reset;
  if (!isfinite(heading_deg) || fabsf(heading_deg) > 180.0f ||
      !isfinite(field_forward) || !isfinite(field_left) ||
      hypotf(field_forward, field_left) > TRACK_MAX_TRANSLATION_RPM ||
      !isfinite(gyro.yaw_deg) || !isfinite(gyro.rate_dps) ||
      !SpeedControl_GetSnapshot().enabled) return 0U;
  reset = !track.active ||
      fabsf(wrap(heading_deg - state.target_deg)) > 0.01f;
  if (!start(2U)) return 0U;
  if (reset)
  {
    memset(&track, 0, sizeof(track));
    track.active = 1U;
    track.damping_dps = gyro.rate_dps;
    integral_output = 0.0f;
  }
  manual.active = 0U;
  state.target_deg = wrap(heading_deg);
  track.field_forward = field_forward;
  track.field_left = field_left;
  /* A zero target must not retain the previous ramp's velocity tail. */
  if (field_forward == 0.0f && field_left == 0.0f)
    track.reference_forward = track.reference_left = 0.0f;
  return 1U;
}
uint8_t AttitudeControl_KeepAlive(void)
{
  if (!state.mode) return 0U;
  command_tick = HAL_GetTick();
  return 1U;
}
uint8_t AttitudeControl_SetRatePid(float kp, float ki)
{
  if (!isfinite(kp) || !isfinite(ki) || kp < 0 || kp > 100 ||
      ki < 0 || ki > 100) return 0U;
  state.rate_kp = kp;
  state.rate_ki = ki;
  /* Bumpless transfer at the current error; preserve the active experiment. */
  integral_output = clamp(state.output_rpm -
      kp * (state.target_dps - state.feedback_dps), ATTITUDE_OUTPUT_LIMIT_RPM);
  return 1U;
}
uint8_t AttitudeControl_SetHeadingPid(float kp, float kd)
{
  if (!isfinite(kp) || !isfinite(kd) || kp < 0 || kp > 100 ||
      kd < 0 || kd > 100) return 0U;
  state.heading_kp = kp;
  state.heading_kd = kd;
  return 1U;
}

static uint8_t update_manual_brake(const GyroSnapshot *gyro, uint32_t elapsed_ms)
{
  manual.brake_ms += elapsed_ms;
  state.target_deg = wrap(gyro->yaw_deg);
  state.error_deg = 0.0f;
  state.output_dps = state.target_dps = 0.0f;
  integral_output = 0.0f;
  if (fabsf(gyro->rate_dps) <= MANUAL_REST_ENTER_DPS &&
      fabsf(manual.damping_dps) <= MANUAL_REST_ENTER_DPS &&
      fabsf(state.output_rpm) < 0.5f)
    manual.settled_ms += elapsed_ms;
  else
    manual.settled_ms = 0U;
  if (manual.settled_ms >= MANUAL_BRAKE_SETTLE_MS)
  {
    manual.braking = 0U;
    manual.resting = 1U;
    manual.damping_dps = gyro->rate_dps;
    return 1U;
  }
  return manual.brake_ms <= MANUAL_BRAKE_TIMEOUT_MS;
}

static uint8_t update_manual_target(const GyroSnapshot *gyro, float dt,
                                    uint32_t elapsed_ms)
{
  float error, correction;

  manual.damping_dps += dt / (MANUAL_D_FILTER_SECONDS + dt) *
      (gyro->rate_dps - manual.damping_dps);
  if (manual.braking) return update_manual_brake(gyro, elapsed_ms);
  manual.reference_dps += clamp(manual.requested_dps - manual.reference_dps,
                                 MANUAL_RATE_ACCEL_DPS2 * dt);
  state.target_deg = wrap(state.target_deg + manual.reference_dps * dt);
  error = wrap(state.target_deg - gyro->yaw_deg);
  if (manual.requested_dps != 0.0f || manual.reference_dps != 0.0f)
  {
    /* Bound angle-reference windup while the chassis cannot follow a stick. */
    state.saturated |= fabsf(error) > MANUAL_TRACK_ERROR_DEG;
    error = clamp(error, MANUAL_TRACK_ERROR_DEG);
    state.target_deg = wrap(gyro->yaw_deg + error);
  }
  state.error_deg = error;
  if (manual.requested_dps == 0.0f && manual.reference_dps == 0.0f)
  {
    if (fabsf(error) > MANUAL_REST_EXIT_DEG ||
        fabsf(manual.damping_dps) > MANUAL_REST_EXIT_DPS)
      manual.resting = 0U;
    else if (fabsf(error) < MANUAL_REST_ENTER_DEG &&
             fabsf(manual.damping_dps) < MANUAL_REST_ENTER_DPS)
      manual.resting = 1U;
  }
  if (manual.resting)
  {
    state.output_dps = state.target_dps = 0.0f;
    return 1U;
  }
  /* Moving heading reference: rate feedforward plus PD tracking.
   * D acts on the rate mismatch, so it does not oppose an intended turn. */
  correction = state.heading_kp * error + state.heading_kd *
      (manual.reference_dps - manual.damping_dps);
  if (manual.requested_dps == 0.0f && manual.reference_dps == 0.0f)
  {
    state.saturated |= fabsf(correction) > MANUAL_HOLD_RATE_DPS;
    correction = clamp(correction, MANUAL_HOLD_RATE_DPS);
  }
  state.output_dps = manual.reference_dps + correction;
  state.target_dps = clamp(state.output_dps, ATTITUDE_RATE_LIMIT_DPS);
  state.saturated |= state.target_dps != state.output_dps;
  state.output_dps = state.target_dps;
  return 1U;
}

static float limit_rate_output(float output, float limit, float dt)
{
  if (manual.active)
    output = state.output_rpm + clamp(output - state.output_rpm,
                                      MANUAL_OUTPUT_SLEW_RPM_S * dt);
  return clamp(output, limit);
}

static void update_tracked_drive(const GyroSnapshot *gyro, float dt,
                                 uint32_t elapsed_ms)
{
  float df, dl, distance, scale, yaw, raw;
  track.damping_dps += dt / (MANUAL_D_FILTER_SECONDS + dt) *
      (gyro->rate_dps - track.damping_dps);
  /* Schmitt gate uses BOTH raw and filtered rate. Never recapture a commanded
   * angle, and never integrate gyro noise into alternating +/-1 RPM commands. */
  if (fabsf(state.error_deg) > MANUAL_REST_EXIT_DEG ||
      fabsf(gyro->rate_dps) > MANUAL_REST_EXIT_DPS)
    track.resting = 0U;
  if (fabsf(state.error_deg) <= MANUAL_REST_ENTER_DEG &&
      fabsf(gyro->rate_dps) <= MANUAL_REST_ENTER_DPS &&
      fabsf(track.damping_dps) <= MANUAL_REST_ENTER_DPS)
    track.settled_ms += elapsed_ms;
  else
    track.settled_ms = 0U;
  if (track.settled_ms >= TRACK_REST_SETTLE_MS)
  {
    track.resting = 1U;
    track.settled_ms = TRACK_REST_SETTLE_MS;
  }
  raw = state.heading_kp * state.error_deg -
        state.heading_kd * track.damping_dps;
  state.target_dps = track.resting ? 0.0f : clamp(raw, ATTITUDE_RATE_LIMIT_DPS);
  state.output_dps = state.target_dps;
  state.saturated |= !track.resting && raw != state.target_dps;

  df = track.field_forward - track.reference_forward;
  dl = track.field_left - track.reference_left;
  distance = hypotf(df, dl);
  scale = distance > 0.0f ? fminf(1.0f,
      TRACK_TRANSLATION_ACCEL_RPM_S * dt / distance) : 0.0f;
  track.reference_forward += df * scale;
  track.reference_left += dl * scale;
  yaw = gyro->yaw_deg * DEGREES_TO_RADIANS;
  forward_rpm = (int16_t)lroundf(track.reference_forward * cosf(yaw) +
                                track.reference_left * sinf(yaw));
  left_rpm = (int16_t)lroundf(-track.reference_forward * sinf(yaw) +
                             track.reference_left * cosf(yaw));
}

static void allocate_translation(uint16_t wheel_limit,
                                         int16_t *forward, int16_t *left)
{
  float translation = (float)(abs((int)*forward) + abs((int)*left));
  /* Reserve the actual integer rotation first; preserve translation direction. */
  float available = fmaxf(0.0f, (float)wheel_limit -
                            fabsf(roundf(state.output_rpm)));
  if (translation > available)
  {
    *forward = (int16_t)((float)*forward * available / translation);
    *left = (int16_t)((float)*left * available / translation);
    state.saturated = 1U;
  }
}

void AttitudeControl_Process(uint32_t elapsed_ms)
{
  GyroSnapshot gyro;
  SpeedControlSnapshot speed;
  float error, candidate, raw, output, limit, dt;
  int16_t applied_forward, applied_left;
  uint32_t sample_ms, sample_us;
  if (!state.mode) return;
  if (!ready() || !SpeedControl_GetSnapshot().enabled ||
      HAL_GetTick() - command_tick > ATTITUDE_WATCHDOG_MS)
  {
    ChassisTest_Stop();
    return;
  }
  accumulated_ms += elapsed_ms;
  if (accumulated_ms < ATTITUDE_PERIOD_MS) return;
  if (accumulated_ms > 60U)
  {
    ChassisTest_Stop();
    return;
  }
  Timebase_Capture(&sample_ms, &sample_us);
  gyro = Gyro_GetSnapshot();
  if (!isfinite(gyro.yaw_deg) || !isfinite(gyro.rate_dps))
  {
    ChassisTest_Stop();
    return;
  }
  dt = (float)accumulated_ms / 1000.0f;
  speed = SpeedControl_GetSnapshot();
  state.feedback_deg = gyro.yaw_deg;
  state.feedback_dps = gyro.rate_dps;
  state.error_deg = wrap(state.target_deg - gyro.yaw_deg);
  state.saturated = 0U;
  if (manual.active)
  {
    if (!update_manual_target(&gyro, dt, accumulated_ms))
    {
      ChassisTest_Stop();
      return;
    }
  }
  else if (track.active)
    update_tracked_drive(&gyro, dt, accumulated_ms);
  else if (state.mode == 2U)
  {
    raw = state.heading_kp * state.error_deg - state.heading_kd * gyro.rate_dps;
    state.output_dps = clamp(raw, ATTITUDE_RATE_LIMIT_DPS);
    state.saturated = fabsf(raw) > ATTITUDE_RATE_LIMIT_DPS;
    state.target_dps = state.output_dps;
  }
  /* Manual and tracked motion reserve rotation; raw HEAD/MOVE experiments
   * retain their original translation-first allocation. */
  limit = (manual.active || track.active) ?
      fminf(ATTITUDE_OUTPUT_LIMIT_RPM, (float)speed.target_limit_rpm) :
      fminf(ATTITUDE_OUTPUT_LIMIT_RPM,
      fmaxf(0, (float)speed.target_limit_rpm -
                    (float)(abs((int)forward_rpm) + abs((int)left_rpm))));
  error = state.target_dps - gyro.rate_dps;
  if (track.active && track.resting)
  {
    integral_output = 0.0f;
    error = 0.0f;
  }
  if (manual.active && (manual.resting || manual.braking))
  {
    integral_output = 0.0f;
    /* P-only zero-rate damping during release cannot accumulate a reverse
     * rate command. Ignore the small rate noise once almost stopped. */
    if (manual.resting || fabsf(gyro.rate_dps) <= MANUAL_REST_ENTER_DPS)
      error = 0.0f;
  }
  candidate = integral_output + state.rate_ki * error * dt;
  if (manual.active && manual.braking) candidate = 0.0f;
  raw = state.rate_kp * error + candidate;
  output = limit_rate_output(raw, limit, dt);
  if ((raw == output) || (raw > output && error < 0) ||
      (raw < output && error > 0) ||
      (manual.active && candidate * integral_output >= 0.0f &&
       fabsf(candidate) < fabsf(integral_output)))
    integral_output = clamp(candidate, limit);
  raw = state.rate_kp * error + integral_output;
  output = limit_rate_output(raw, limit, dt);
  state.saturated |= raw != output;
  state.output_rpm = output;
  applied_forward = forward_rpm;
  applied_left = left_rpm;
  if (manual.active || track.active)
    allocate_translation(speed.target_limit_rpm,
                                  &applied_forward, &applied_left);
  (void)SpeedControl_SetDrive(applied_forward, applied_left,
                              (int16_t)lroundf(state.output_rpm));
  state.forward_rpm = applied_forward;
  state.left_rpm = applied_left;
  Telemetry_Hlog_Record(&state, sample_ms, sample_us);
  accumulated_ms = 0U;
}
AttitudeControlSnapshot AttitudeControl_GetSnapshot(void) { return state; }
