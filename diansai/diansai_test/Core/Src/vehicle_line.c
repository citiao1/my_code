#include "vehicle_line.h"

#include "main.h"
#include "vehicle_config.h"
#include "vehicle_buzzer.h"
#include "vehicle_gray.h"
#include "vehicle_internal.h"
#include "vehicle_motor.h"

#include <math.h>

static const int8_t sensor_weights[GRAY_SENSOR_CHANNELS] = {7, 5, 3, 1, -1, -3, -5, -7};
static const uint16_t default_gray_white[GRAY_SENSOR_CHANNELS] =
  {978U, 755U, 607U, 536U, 518U, 656U, 642U, 756U};
static const uint16_t default_gray_black[GRAY_SENSOR_CHANNELS] =
  {4024U, 4023U, 4027U, 4025U, 4025U, 4024U, 4023U, 4026U};

static uint16_t NormalizeGray(uint8_t channel)
{
  uint16_t white = line_follow.gray_white[channel];
  uint16_t black = line_follow.gray_black[channel];
  uint16_t raw = line_follow.gray[channel];
  uint32_t normalized;

  if (black <= white + GRAY_CALIBRATION_MIN_SPAN_ADC) return 0U;
  if (raw <= white) return 0U;
  if (raw >= black) return GRAY_NORMALIZED_MAX;
  normalized = (uint32_t)(raw - white) * GRAY_NORMALIZED_MAX / (black - white);
  return (uint16_t)normalized;
}

static void ReadCalibrationAverage(uint16_t values[GRAY_SENSOR_CHANNELS])
{
  uint8_t sample;
  uint8_t channel;
  uint16_t readings[GRAY_SENSOR_CHANNELS];
  uint32_t sums[GRAY_SENSOR_CHANNELS] = {0U};

  for (sample = 0U; sample < GRAY_CALIBRATION_SAMPLES; ++sample)
  {
    VehicleGray_ReadAll(readings);
    for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
      sums[channel] += readings[channel];
  }
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
    values[channel] = (uint16_t)(sums[channel] / GRAY_CALIBRATION_SAMPLES);
}

static void StopMotion(LineFollowPhase phase)
{
  line_follow.phase = phase;
  line_follow.phase_start_ms = HAL_GetTick();
  line_follow.target_yaw_rate = 0.0f;
  line_follow.lost_start_ms = 0U;
  line_follow.corner_detect_count = 0U;
  line_follow.corner_settle_start_ms = 0U;
  state.throttle = 0;
  state.steering = 0;
  VehicleMotor_Stop();
}

static void StartWait(uint32_t now)
{
  square_test.active = 0U;
  square_test.phase = SQUARE_PHASE_IDLE;
  line_follow.phase = LINE_PHASE_WAIT;
  line_follow.phase_start_ms = now;
  line_follow.lost_start_ms = 0U;
  line_follow.target_yaw_rate = 0.0f;
  line_follow.previous_error = line_follow.filtered_error;
  line_follow.corner_detect_count = 0U;
  line_follow.corner_settle_start_ms = 0U;
  state.throttle = 0;
  state.steering = 0;
  VehicleMotor_Stop();
}

static void HandleButton(uint32_t now)
{
  uint8_t pressed = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET ? 1U : 0U;

  if (pressed != line_follow.button_raw)
  {
    line_follow.button_raw = pressed;
    line_follow.button_change_ms = now;
  }

  if (pressed != line_follow.button_stable &&
      (now - line_follow.button_change_ms) >= LINE_BUTTON_DEBOUNCE_MS)
  {
    line_follow.button_stable = pressed;
    if (pressed)
    {
      if (VehicleLine_IsControlActive())
      {
        StopMotion(LINE_PHASE_IDLE);
      }
      else
      {
        StartWait(now);
      }
    }
  }
}

static void UpdateLinePosition(void)
{
  uint8_t channel;
  uint8_t was_visible = line_follow.line_visible;
  uint16_t minimum;
  uint16_t maximum;
  uint16_t contrast;
  uint32_t strength_sum = 0U;
  int32_t weighted_sum = 0;
  uint8_t current_corner_run = 0U;
  uint8_t longest_corner_run = 0U;

  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
    line_follow.gray_normalized[channel] = NormalizeGray(channel);

  minimum = line_follow.gray_normalized[0];
  maximum = line_follow.gray_normalized[0];
  for (channel = 1U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    if (line_follow.gray_normalized[channel] < minimum)
      minimum = line_follow.gray_normalized[channel];
    if (line_follow.gray_normalized[channel] > maximum)
      maximum = line_follow.gray_normalized[channel];
  }

  contrast = maximum - minimum;

  line_follow.active_count = 0U;
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    uint16_t strength;
    uint16_t white_delta = line_follow.gray[channel] > line_follow.gray_white[channel] ?
                           line_follow.gray[channel] - line_follow.gray_white[channel] : 0U;
    strength = line_follow.gray_normalized[channel] > GRAY_LINE_THRESHOLD_NORMALIZED ?
               line_follow.gray_normalized[channel] - GRAY_LINE_THRESHOLD_NORMALIZED : 0U;
    if (white_delta >= GRAY_CORNER_BLACK_DELTA_ADC &&
        line_follow.gray_normalized[channel] >= GRAY_CORNER_BLACK_NORMALIZED)
    {
      ++line_follow.active_count;
      ++current_corner_run;
      if (current_corner_run > longest_corner_run) longest_corner_run = current_corner_run;
    }
    else
    {
      current_corner_run = 0U;
    }
    strength_sum += strength;
    weighted_sum += (int32_t)sensor_weights[channel] * (int32_t)strength;
  }

  line_follow.corner_contiguous_count = longest_corner_run;

  line_follow.line_visible =
    (contrast >= GRAY_LINE_MIN_CONTRAST_NORMALIZED && strength_sum > 0U) ? 1U : 0U;

  if (line_follow.line_visible)
  {
    float measured_error = (float)weighted_sum * 100.0f / (7.0f * (float)strength_sum);
#if GRAY_SENSOR_REVERSED
    measured_error = -measured_error;
#endif
    if (!was_visible)
    {
      line_follow.filtered_error = measured_error;
      line_follow.previous_error = measured_error;
    }
    else
    {
      line_follow.filtered_error +=
        LINE_ERROR_FILTER_ALPHA * (measured_error - line_follow.filtered_error);
    }
    line_follow.error = line_follow.filtered_error;
    if (fabsf(line_follow.error) >= 2.0f) line_follow.last_seen_error = line_follow.error;
  }
}

static void UpdateRunCommand(uint32_t now)
{
  float yaw_target;
  float absolute_error;
  int throttle;

  if (line_follow.corner_cooldown_start_ms != 0U &&
      (!line_follow.line_visible ||
       line_follow.active_count >= LINE_CORNER_ACTIVE_CHANNELS))
  {
    if ((now - line_follow.corner_cooldown_start_ms) >= LINE_CORNER_EXIT_TIMEOUT_MS)
    {
      StopMotion(LINE_PHASE_LOST);
      return;
    }
    line_follow.lost_start_ms = 0U;
    line_follow.target_yaw_rate = 0.0f;
    state.throttle = line_follow.speed_percent < LINE_CORNER_ADVANCE_THROTTLE_PERCENT ?
                     (int8_t)line_follow.speed_percent : LINE_CORNER_ADVANCE_THROTTLE_PERCENT;
    state.steering = 0;
    state.heading_hold_active = 0U;
    return;
  }

  if (line_follow.line_visible)
  {
    float derivative = line_follow.error - line_follow.previous_error;
    line_follow.lost_start_ms = 0U;
    yaw_target = line_follow.direction_kp * line_follow.error +
                 line_follow.direction_kd * derivative;
    line_follow.previous_error = line_follow.error;
  }
  else
  {
    if (line_follow.lost_start_ms == 0U) line_follow.lost_start_ms = now;
    if ((now - line_follow.lost_start_ms) >= LINE_LOST_STOP_MS)
    {
      StopMotion(LINE_PHASE_LOST);
      return;
    }
    yaw_target = line_follow.last_seen_error >= 0.0f ?
                 LINE_LOST_SEARCH_YAW_DPS : -LINE_LOST_SEARCH_YAW_DPS;
  }

  if (yaw_target > LINE_MAX_YAW_RATE_DPS) yaw_target = LINE_MAX_YAW_RATE_DPS;
  if (yaw_target < -LINE_MAX_YAW_RATE_DPS) yaw_target = -LINE_MAX_YAW_RATE_DPS;
  line_follow.target_yaw_rate = yaw_target;

  absolute_error = fabsf(line_follow.error);
  if (absolute_error > 100.0f) absolute_error = 100.0f;
  throttle = (int)line_follow.speed_percent -
             (int)((float)((int)line_follow.speed_percent - LINE_MIN_THROTTLE_PERCENT) *
                   absolute_error / 100.0f);
  if (!line_follow.line_visible) throttle = LINE_MIN_THROTTLE_PERCENT;
  state.throttle = (int8_t)throttle;
  state.steering = 0;
  state.heading_hold_active = 0U;
}

static void BeginCornerAdvance(uint32_t now)
{
  line_follow.phase = LINE_PHASE_CORNER_ADVANCE;
  line_follow.phase_start_ms = now;
  line_follow.corner_start_left = state.encoder_left;
  line_follow.corner_start_right = state.encoder_right;
  line_follow.corner_detect_count = 0U;
  line_follow.target_yaw_rate = 0.0f;
  state.throttle = line_follow.speed_percent < LINE_CORNER_ADVANCE_THROTTLE_PERCENT ?
                   (int8_t)line_follow.speed_percent : LINE_CORNER_ADVANCE_THROTTLE_PERCENT;
  state.steering = 0;
  state.heading_hold_active = 0U;
}

static void BeginCornerTurn(uint32_t now)
{
  VehicleMotor_Stop();
  state.heading_control_enabled =
    (state.mpu_ok && state.yaw_control_enabled) ? 1U : 0U;
  line_follow.phase = LINE_PHASE_CORNER_TURN;
  line_follow.phase_start_ms = now;
  line_follow.corner_settle_start_ms = 0U;
  line_follow.target_yaw_rate = 0.0f;
  line_follow.corner_target_heading =
    Vehicle_WrapAngle(state.yaw + line_follow.corner_turn_angle_deg);
  state.throttle = 0;
  state.steering = 0;
  state.target_heading = line_follow.corner_target_heading;
  state.heading_reference = state.yaw;
  state.heading_reference_rate = 0.0f;
  state.heading_error = Vehicle_WrapAngle(state.target_heading - state.yaw);
  state.heading_output = 0.0f;
  state.heading_hold_active = 1U;
  last_heading_ms = now;
}

static void UpdateCornerAdvance(uint32_t now)
{
  uint32_t timeout_ms = LINE_CORNER_ADVANCE_TIMEOUT_MS +
                        (uint32_t)VehicleLine_GetCornerAdvanceMm() *
                        LINE_CORNER_ADVANCE_TIMEOUT_PER_MM_MS;
  float left_distance = (float)(state.encoder_left - line_follow.corner_start_left) /
                        LEFT_COUNTS_PER_METER;
  float right_distance = (float)(state.encoder_right - line_follow.corner_start_right) /
                         RIGHT_COUNTS_PER_METER;
  float progress = 0.5f * (left_distance + right_distance);

  line_follow.target_yaw_rate = 0.0f;
  state.throttle = line_follow.speed_percent < LINE_CORNER_ADVANCE_THROTTLE_PERCENT ?
                   (int8_t)line_follow.speed_percent : LINE_CORNER_ADVANCE_THROTTLE_PERCENT;
  state.steering = 0;
  state.heading_hold_active = 0U;
  if (progress >= line_follow.corner_advance_distance_m ||
      (now - line_follow.phase_start_ms) >= timeout_ms)
  {
    BeginCornerTurn(now);
  }
}

static void UpdateCornerTurn(uint32_t now)
{
  float heading_error = Vehicle_WrapAngle(line_follow.corner_target_heading - state.yaw);

  if (!state.heading_control_enabled)
  {
    StopMotion(LINE_PHASE_ERROR);
    return;
  }

  if (fabsf(heading_error) <= LINE_CORNER_HEADING_TOLERANCE_DEG &&
      fabsf(state.yaw_rate) <= LINE_CORNER_YAW_RATE_TOLERANCE_DPS)
  {
    if (line_follow.corner_settle_start_ms == 0U) line_follow.corner_settle_start_ms = now;
    if ((now - line_follow.corner_settle_start_ms) >= LINE_CORNER_SETTLE_MS)
    {
      VehicleMotor_Stop();
      line_follow.phase = LINE_PHASE_RUN;
      line_follow.phase_start_ms = now;
      line_follow.corner_cooldown_start_ms = now;
      line_follow.corner_settle_start_ms = 0U;
      line_follow.corner_detect_count = 0U;
      line_follow.lost_start_ms = 0U;
      line_follow.target_yaw_rate = 0.0f;
      line_follow.previous_error = line_follow.error;
      state.throttle = 0;
      state.steering = 0;
      return;
    }
  }
  else
  {
    line_follow.corner_settle_start_ms = 0U;
  }

  if ((now - line_follow.phase_start_ms) >= LINE_CORNER_TURN_TIMEOUT_MS)
  {
    StopMotion(LINE_PHASE_ERROR);
  }
}

static uint8_t DetectCorner(uint32_t now, uint8_t sensor_updated)
{
  if (!sensor_updated) return 0U;
  if (line_follow.corner_cooldown_start_ms != 0U)
  {
    if ((now - line_follow.corner_cooldown_start_ms) < LINE_CORNER_COOLDOWN_MS ||
        !line_follow.line_visible ||
        line_follow.active_count >= LINE_CORNER_ACTIVE_CHANNELS)
    {
      line_follow.corner_detect_count = 0U;
      return 0U;
    }
    line_follow.corner_cooldown_start_ms = 0U;
  }

  if (line_follow.active_count >= LINE_CORNER_ACTIVE_CHANNELS &&
      line_follow.corner_contiguous_count >= LINE_CORNER_CONTIGUOUS_CHANNELS)
  {
    if (line_follow.corner_detect_count < LINE_CORNER_CONFIRM_SAMPLES)
      ++line_follow.corner_detect_count;
  }
  else if (line_follow.corner_detect_count > 0U)
  {
    --line_follow.corner_detect_count;
  }

  if (line_follow.corner_detect_count >= LINE_CORNER_CONFIRM_SAMPLES)
  {
    VehicleBuzzer_Beep(LINE_CORNER_BEEP_MS);
    BeginCornerAdvance(now);
    return 1U;
  }
  return 0U;
}

void VehicleLine_Init(uint32_t now)
{
  uint8_t channel;
  line_follow.phase = LINE_PHASE_IDLE;
  line_follow.speed_percent = LINE_DEFAULT_SPEED_PERCENT;
  line_follow.corner_advance_distance_m = LINE_CORNER_ADVANCE_DISTANCE_M;
  line_follow.corner_turn_angle_deg = LINE_CORNER_TURN_DEG;
  line_follow.direction_kp = LINE_DIRECTION_KP;
  line_follow.direction_kd = LINE_DIRECTION_KD;
  line_follow.button_raw =
    HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET ? 1U : 0U;
  line_follow.button_stable = line_follow.button_raw;
  line_follow.button_change_ms = now;
  line_follow.last_sample_ms = now - LINE_IDLE_SAMPLE_PERIOD_MS;
  VehicleGray_ReadAll(line_follow.gray);
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    line_follow.gray_white[channel] = default_gray_white[channel];
    line_follow.gray_black[channel] = default_gray_black[channel];
  }
  line_follow.white_calibrated = 1U;
  line_follow.black_calibrated = 1U;
  UpdateLinePosition();
}

void VehicleLine_Update(uint32_t now)
{
  uint32_t sample_period;
  uint8_t sensor_updated = 0U;

  HandleButton(now);
  sample_period = VehicleLine_IsControlActive() && line_follow.phase != LINE_PHASE_WAIT ?
                  LINE_SAMPLE_PERIOD_MS : LINE_IDLE_SAMPLE_PERIOD_MS;
  if ((now - line_follow.last_sample_ms) >= sample_period)
  {
    line_follow.last_sample_ms = now;
    VehicleGray_ReadAll(line_follow.gray);
    UpdateLinePosition();
    sensor_updated = 1U;
  }

  if (line_follow.phase == LINE_PHASE_WAIT)
  {
    state.throttle = 0;
    state.steering = 0;
    if ((now - line_follow.phase_start_ms) >= LINE_START_DELAY_MS)
    {
      if (!VehicleMotor_IsEnabled() || !state.mpu_ok || !state.yaw_control_enabled)
      {
        StopMotion(LINE_PHASE_ERROR);
      }
      else if (!line_follow.line_visible)
      {
        StopMotion(LINE_PHASE_LOST);
      }
      else
      {
        line_follow.phase = LINE_PHASE_RUN;
        line_follow.phase_start_ms = now;
        line_follow.previous_error = line_follow.error;
        line_follow.last_seen_error = line_follow.error;
        line_follow.lost_start_ms = 0U;
      }
    }
  }
  else if (line_follow.phase == LINE_PHASE_RUN)
  {
    if (!VehicleMotor_IsEnabled() || !state.mpu_ok || !state.yaw_control_enabled)
    {
      StopMotion(LINE_PHASE_ERROR);
    }
    else
    {
      if (!DetectCorner(now, sensor_updated)) UpdateRunCommand(now);
    }
  }
  else if (line_follow.phase == LINE_PHASE_CORNER_ADVANCE)
  {
    if (!VehicleMotor_IsEnabled() || !state.mpu_ok || !state.yaw_control_enabled)
    {
      StopMotion(LINE_PHASE_ERROR);
    }
    else
    {
      UpdateCornerAdvance(now);
    }
  }
  else if (line_follow.phase == LINE_PHASE_CORNER_TURN)
  {
    if (!VehicleMotor_IsEnabled() || !state.mpu_ok || !state.yaw_control_enabled)
    {
      StopMotion(LINE_PHASE_ERROR);
    }
    else
    {
      UpdateCornerTurn(now);
    }
  }
}

void VehicleLine_Stop(void)
{
  StopMotion(LINE_PHASE_IDLE);
}

void VehicleLine_SetSpeedPercent(uint8_t percent)
{
  if (percent < 20U) percent = 20U;
  if (percent > 100U) percent = 100U;
  line_follow.speed_percent = percent;
}

uint8_t VehicleLine_GetSpeedPercent(void)
{
  return line_follow.speed_percent;
}

void VehicleLine_SetDirectionGains(float kp, float kd)
{
  if (kp < 0.0f) kp = 0.0f;
  if (kp > LINE_DIRECTION_KP_MAX) kp = LINE_DIRECTION_KP_MAX;
  if (kd < 0.0f) kd = 0.0f;
  if (kd > LINE_DIRECTION_KD_MAX) kd = LINE_DIRECTION_KD_MAX;
  line_follow.direction_kp = kp;
  line_follow.direction_kd = kd;
}

float VehicleLine_GetDirectionKp(void)
{
  return line_follow.direction_kp;
}

float VehicleLine_GetDirectionKd(void)
{
  return line_follow.direction_kd;
}

void VehicleLine_CaptureWhite(void)
{
  uint8_t channel;
  ReadCalibrationAverage(line_follow.gray);
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    line_follow.gray_white[channel] = line_follow.gray[channel];
    line_follow.gray_black[channel] = 4095U;
  }
  line_follow.white_calibrated = 1U;
  line_follow.black_calibrated = 0U;
  UpdateLinePosition();
}

uint8_t VehicleLine_CaptureBlack(void)
{
  uint8_t channel;
  uint16_t readings[GRAY_SENSOR_CHANNELS];

  if (!line_follow.white_calibrated) return 0U;
  ReadCalibrationAverage(readings);
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    if (readings[channel] <= line_follow.gray_white[channel] +
                             GRAY_CALIBRATION_MIN_SPAN_ADC)
      return 0U;
  }
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    line_follow.gray[channel] = readings[channel];
    line_follow.gray_black[channel] = readings[channel];
  }
  line_follow.black_calibrated = 1U;
  UpdateLinePosition();
  return 1U;
}

void VehicleLine_SetCornerAdvanceMm(uint16_t millimeters)
{
  if (millimeters < LINE_CORNER_ADVANCE_MIN_MM) millimeters = LINE_CORNER_ADVANCE_MIN_MM;
  if (millimeters > LINE_CORNER_ADVANCE_MAX_MM) millimeters = LINE_CORNER_ADVANCE_MAX_MM;
  line_follow.corner_advance_distance_m = (float)millimeters / 1000.0f;
}

uint16_t VehicleLine_GetCornerAdvanceMm(void)
{
  return (uint16_t)(line_follow.corner_advance_distance_m * 1000.0f + 0.5f);
}

void VehicleLine_SetCornerTurnDeg(uint16_t degrees)
{
  if (degrees < LINE_CORNER_TURN_MIN_DEG) degrees = LINE_CORNER_TURN_MIN_DEG;
  if (degrees > LINE_CORNER_TURN_MAX_DEG) degrees = LINE_CORNER_TURN_MAX_DEG;
  line_follow.corner_turn_angle_deg = (float)degrees;
}

uint16_t VehicleLine_GetCornerTurnDeg(void)
{
  return (uint16_t)(line_follow.corner_turn_angle_deg + 0.5f);
}

uint8_t VehicleLine_IsEngaged(void)
{
  return line_follow.phase != LINE_PHASE_IDLE;
}

uint8_t VehicleLine_IsControlActive(void)
{
  return line_follow.phase == LINE_PHASE_WAIT || line_follow.phase == LINE_PHASE_RUN ||
         line_follow.phase == LINE_PHASE_CORNER_ADVANCE ||
         line_follow.phase == LINE_PHASE_CORNER_TURN;
}

uint8_t VehicleLine_IsRunning(void)
{
  return line_follow.phase == LINE_PHASE_RUN || line_follow.phase == LINE_PHASE_CORNER_ADVANCE;
}

float VehicleLine_GetTargetYawRate(void)
{
  return line_follow.target_yaw_rate;
}
