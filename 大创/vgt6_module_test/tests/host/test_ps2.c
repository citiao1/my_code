/* Real PS2 driver, control, attitude cascade and speed loops. Only the
 * physical receiver, IMU, encoders and motor I/O are simulated. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "ps2.h"
#include "ps2_control.h"
#include "chassis_test.h"
#include "speed_control.h"
#include "attitude_control.h"
#include "gyro.h"
#include "encoder.h"
#include "telemetry.h"
#include "tof.h"

HostDwt host_dwt;
HostCoreDebug host_debug;
GPIO_TypeDef host_gpio;
uint32_t SystemCoreClock = 168000000U;
static uint32_t tick, fall_cycle, last_byte_cycle;
static uint8_t response[21], sent[21];
static unsigned bit_index, transaction_bytes, transactions;
static uint8_t selected, clock_high = 1U, command_high = 1U, was_read;
static unsigned stop_count;
static uint8_t detach_in_delay;
static GyroSnapshot gyro;
static EncoderSnapshot encoder;
#define enabled (SpeedControl_GetSnapshot().enabled)
#define forward (Ps2Control_GetSnapshot().forward_rpm)
#define left (Ps2Control_GetSnapshot().left_rpm)
#define rotate (Ps2Control_GetSnapshot().rotate_rpm)

uint32_t HAL_GetTick(void) { return tick; }
void Host_AdvanceCycles(void)
{
  if (detach_in_delay)
  {
    host_debug.DEMCR = host_dwt.CTRL = 0U;
    detach_in_delay = 0U;
  }
  if ((host_debug.DEMCR & CoreDebug_DEMCR_TRCENA_Msk) &&
      (host_dwt.CTRL & DWT_CTRL_CYCCNTENA_Msk))
    host_dwt.CYCCNT += 168U;
}
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *gpio)
{ (void)port; (void)gpio; }
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState value)
{
  (void)port;
  if (pin == PS2_SEL_Pin)
  {
    if (value == GPIO_PIN_RESET)
    {
      assert(!selected && clock_high);
      selected = 1U;
      bit_index = 0U;
      was_read = 0U;
      memset(sent, 0, sizeof(sent));
    }
    else if (selected)
    {
      assert(bit_index % 8U == 0U && clock_high);
      transaction_bytes = bit_index / 8U;
      ++transactions;
      selected = 0U;
    }
  }
  if (pin == PS2_CMD_Pin)
  {
    if (selected) assert(clock_high);
    command_high = value != GPIO_PIN_RESET;
  }
  if (pin == PS2_CLK_Pin)
  {
    if (selected && clock_high && value == GPIO_PIN_RESET)
    {
      assert(bit_index < sizeof(response) * 8U);
      if (bit_index > 0U && bit_index % 8U == 0U)
        assert(host_dwt.CYCCNT - last_byte_cycle >= 16U * 168U);
      fall_cycle = host_dwt.CYCCNT;
      if (command_high) sent[bit_index / 8U] |= 1U << (bit_index % 8U);
    }
    if (selected && !clock_high && value == GPIO_PIN_SET)
    {
      assert(was_read);
      was_read = 0U;
      ++bit_index;
      if (bit_index % 8U == 0U) last_byte_cycle = host_dwt.CYCCNT;
    }
    clock_high = value != GPIO_PIN_RESET;
  }
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
  (void)port;
  assert(pin == PS2_DAT_Pin && selected && !clock_high);
  assert(host_dwt.CYCCNT - fall_cycle >= 5U * 168U);
  was_read = 1U;
  return response[bit_index / 8U] & (1U << (bit_index % 8U)) ?
         GPIO_PIN_SET : GPIO_PIN_RESET;
}
uint8_t Motor_Init(void) { return 1U; }
void Motor_StopAll(void) { ++stop_count; }
void Motor_Stop(MotorChannel channel) { (void)channel; }
uint8_t Motor_Run(MotorChannel channel, int8_t direction, uint8_t duty)
{ (void)channel; (void)direction; (void)duty; return 1U; }
uint8_t Motor_Recover(MotorChannel channel) { (void)channel; return 1U; }
GyroSnapshot Gyro_GetSnapshot(void) { return gyro; }
EncoderSnapshot Encoder_GetSnapshot(void) { return encoder; }
void Telemetry_Hlog_Record(const AttitudeControlSnapshot *snapshot,
                           uint32_t ms, uint32_t us)
{ (void)snapshot; (void)ms; (void)us; }
TofSnapshot Tof_GetSnapshot(void) { TofSnapshot s = {0}; return s; }
void Telemetry_Wlog_Record(const SpeedControlSnapshot *s, const EncoderSnapshot *e,
                           uint32_t ms, uint32_t us)
{ (void)s; (void)e; (void)ms; (void)us; }
static void poll(void) { tick += 20U; Ps2_Process(); }
static void buttons(uint16_t pressed)
{
  response[3] = (uint8_t)~pressed;
  response[4] = (uint8_t)(~pressed >> 8);
  poll();
  Ps2Control_Process();
  AttitudeControl_Process(20U);
  SpeedControl_Process(10U);
}

static void reset_manual(float heading)
{
  ChassisTest_Arm();
  assert(SpeedControl_Enable(1U));
  gyro.yaw_deg = heading;
  gyro.rate_dps = 0.0f;
  response[5] = response[7] = response[8] = 128U;
  buttons(0);
}

static void test_continuous_translation_and_hold(void)
{
  float target;
  int previous_left = -1;
  unsigned stops;

  reset_manual(25.0f);
  target = AttitudeControl_GetSnapshot().target_deg;
  stops = stop_count;

  /* Sweep a shallow arc that an independent X deadzone used to flatten. */
  response[8] = 0U;
  for (unsigned raw = 128U; raw >= 88U; raw -= 4U)
  {
    response[7] = (uint8_t)raw;
    buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
    assert(forward > 0 && left >= previous_left);
    assert(abs(forward) + abs(left) <= 140);
    assert(AttitudeControl_GetSnapshot().mode == 2U);
    assert(AttitudeControl_GetSnapshot().target_deg == target);
    previous_left = left;
    if (raw == 124U) assert(left > 0); /* Small cross-axis input survives. */
  }
  response[7] = 96U; response[8] = 32U; /* 1:3 oblique, not 45 degrees. */
  buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
  assert(left > 0 && abs(forward - 3 * left) <= 3);
  response[7] = 160U; response[8] = 224U;
  buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
  assert(left < 0 && forward < 0 && abs(forward - 3 * left) <= 3);

  /* Direction remains fixed while translation and neutral sticks change. */
  gyro.yaw_deg = target - 2.0f;
  response[7] = response[8] = 128U;
  buttons(0);
  assert(AttitudeControl_GetSnapshot().target_deg == target);
  assert(AttitudeControl_GetSnapshot().error_deg == 2.0f);
  assert(AttitudeControl_GetSnapshot().output_rpm > 0.0f);
  assert(SpeedControl_GetSnapshot().target_rpm[0] > 0);
  assert(SpeedControl_GetSnapshot().target_rpm[1] < 0);

  /* Turning/releasing stays in the same cascade without resetting wheels. */
  response[5] = 0U;
  for (unsigned i = 0; i < 12U; ++i)
    buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
  assert(AttitudeControl_GetSnapshot().mode == 2U);
  assert(AttitudeControl_GetSnapshot().target_dps == 80.0f);
  gyro.yaw_deg = 70.0f;
  response[5] = 128U;
  for (unsigned i = 0; i < 30U; ++i) buttons(0);
  target = AttitudeControl_GetSnapshot().target_deg;
  assert(target == 70.0f); /* Capture measured stop, not predicted stop. */
  gyro.yaw_deg = target - 1.0f;
  for (unsigned i = 0; i < 20U; ++i) buttons(0);
  assert(AttitudeControl_GetSnapshot().target_deg == target);
  assert(fabsf(AttitudeControl_GetSnapshot().error_deg - 1.0f) < 0.001f);
  response[5] = 255U;
  for (unsigned i = 0; i < 25U; ++i)
    buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
  assert(AttitudeControl_GetSnapshot().target_dps == -80.0f);
  assert(stop_count == stops);

  reset_manual(179.0f);
  gyro.yaw_deg = -179.0f;
  buttons(0);
  assert(AttitudeControl_GetSnapshot().error_deg == -2.0f);
  /* High-speed diagonal translation still leaves yaw correction headroom. */
  response[7] = response[8] = 0U;
  gyro.yaw_deg = 160.0f;
  for (unsigned i = 0; i < 60U; ++i)
    buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
  assert(abs(forward) + abs(left) <= 140);
  assert(AttitudeControl_GetSnapshot().output_rpm > 0.0f);
  for (unsigned i = 0; i < 4U; ++i)
    assert(abs(SpeedControl_GetSnapshot().target_rpm[i]) <= 160);
  response[7] = response[8] = 128U;
  gyro.yaw_deg = 179.0f;
}

static void test_manual_authority_and_noise(void)
{
  float target;
  unsigned stops;
  /* Every translation tier has the same full-stick angular authority. */
  const uint16_t tiers[] = {0U, PS2_BUTTON_L2,
                           PS2_BUTTON_L2 | PS2_BUTTON_R2};
  for (unsigned tier = 0U; tier < 3U; ++tier)
  {
    reset_manual(0.0f);
    response[5] = 0U;
    response[8] = 0U;
    for (unsigned i = 0U; i < 60U; ++i)
    {
      float previous = AttitudeControl_GetSnapshot().output_rpm;
      buttons(tiers[tier]);
      assert(fabsf(AttitudeControl_GetSnapshot().output_rpm - previous) <=
             4.801f);
    }
    assert(AttitudeControl_GetSnapshot().target_dps == 80.0f);
    assert(AttitudeControl_GetSnapshot().output_rpm > 40.0f);
    if (tier == 2U)
    {
      assert(forward == 140); /* Unmodified joystick request. */
      assert(AttitudeControl_GetSnapshot().forward_rpm < 120);
      assert(AttitudeControl_GetSnapshot().forward_rpm +
             abs((int)lroundf(AttitudeControl_GetSnapshot().output_rpm)) <= 160);
    }
    for (unsigned i = 0U; i < 4U; ++i)
      assert(abs(SpeedControl_GetSnapshot().target_rpm[i]) <= 160);
    /* Long blocked turn stays within eight degrees of measured heading. */
    assert(fabsf(AttitudeControl_GetSnapshot().error_deg) <= 8.001f);
    response[5] = 128U;
    for (unsigned i = 0U; i < 30U; ++i) buttons(tiers[tier]);
    target = AttitudeControl_GetSnapshot().target_deg;
    assert(target >= 0.0f && target < 6.0f); /* No queued full turn. */
    for (unsigned i = 0U; i < 20U; ++i) buttons(tiers[tier]);
    assert(AttitudeControl_GetSnapshot().target_deg == target);
  }

  reset_manual(12.0f);
  target = AttitudeControl_GetSnapshot().target_deg;
  stops = stop_count;
  response[8] = 0U;
  /* Small yaw/rate noise and stick drift cannot alternate wheel correction. */
  for (unsigned i = 0U; i < 200U; ++i)
  {
    gyro.yaw_deg = target + ((i % 2U) ? 0.2f : -0.2f);
    gyro.rate_dps = (i % 2U) ? 1.2f : -1.2f;
    response[5] = (i % 2U) ? 108U : 148U; /* Inside entry hysteresis. */
    buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
    assert(AttitudeControl_GetSnapshot().target_deg == target);
    assert(AttitudeControl_GetSnapshot().output_rpm == 0.0f);
    for (unsigned wheel = 0U; wheel < 4U; ++wheel)
      assert(SpeedControl_GetSnapshot().target_rpm[wheel] == -140);
  }
  gyro.yaw_deg = target - 1.0f;
  gyro.rate_dps = 0.0f;
  response[5] = 128U;
  buttons(0);
  assert(AttitudeControl_GetSnapshot().output_rpm > 0.0f);
  gyro.yaw_deg = target;
  for (unsigned i = 0U; i < 30U; ++i) buttons(0);
  assert(AttitudeControl_GetSnapshot().output_rpm == 0.0f);
  assert(stop_count == stops);
  reset_manual(0.0f);
}

/* Synthetic first-order yaw plant, not a model identified from this vehicle.
 * Exercises the real controller with moving feedback, including quantization. */
static void model_step(void)
{
  SpeedControlSnapshot speed;
  buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2);
  speed = SpeedControl_GetSnapshot();
  gyro.rate_dps += (1.5f * roundf(AttitudeControl_GetSnapshot().output_rpm) -
                     gyro.rate_dps) * (0.02f / 0.12f);
  gyro.yaw_deg += gyro.rate_dps * 0.02f;
  gyro.yaw_deg -= 360.0f * floorf((gyro.yaw_deg + 180.0f) / 360.0f);
  for (unsigned i = 0U; i < 4U; ++i)
    encoder.rpm_x10[i] = speed.target_rpm[i] * 10;
}

static void test_manual_moving_feedback(void)
{
  float held;
  unsigned stops;
  reset_manual(170.0f);
  stops = stop_count;
  response[8] = 0U;
  for (unsigned direction = 0U; direction < 2U; ++direction)
  {
    response[5] = direction == 0U ? 0U : 255U;
    for (unsigned i = 0U; i < 120U; ++i) model_step();
    assert(direction == 0U ? gyro.rate_dps > 65.0f : gyro.rate_dps < -65.0f);
    response[5] = 128U;
    for (unsigned i = 0U; i < 150U; ++i) model_step();
    held = AttitudeControl_GetSnapshot().target_deg;
    assert(fabsf(gyro.rate_dps) < 3.0f);
    assert(fabsf(AttitudeControl_GetSnapshot().error_deg) < 0.9f);
    for (unsigned i = 0U; i < 50U; ++i)
    {
      model_step();
      assert(AttitudeControl_GetSnapshot().target_deg == held);
      assert(AttitudeControl_GetSnapshot().output_rpm == 0.0f);
    }
    gyro.yaw_deg -= 5.0f;
    for (unsigned i = 0U; i < 200U; ++i) model_step();
    assert(AttitudeControl_GetSnapshot().target_deg == held);
    assert(fabsf(AttitudeControl_GetSnapshot().error_deg) < 0.9f);
    assert(fabsf(gyro.rate_dps) < 3.0f);
  }
  assert(stop_count == stops);
  memset(&encoder, 0, sizeof(encoder));
  reset_manual(0.0f);
}

typedef struct { float yaw, rate, gain, tau, feedback_rc; } YawPlant;
static void turn_step(YawPlant *plant, float request, int16_t translation)
{
  tick += 20U;
  assert(AttitudeControl_SetManualDrive(request, translation, 0));
  AttitudeControl_Process(20U);
  assert(enabled);
  plant->rate += (plant->gain * roundf(AttitudeControl_GetSnapshot().output_rpm) -
                   plant->rate) * (0.02f / plant->tau);
  plant->yaw += plant->rate * 0.02f;
  gyro.yaw_deg = plant->yaw - 360.0f * floorf((plant->yaw + 180.0f) / 360.0f);
  gyro.rate_dps += 0.02f / (plant->feedback_rc + 0.02f) * (plant->rate - gyro.rate_dps);
}

/* A sweep of assumed plants, not on-car identification. Test the complete
 * release path, not just eventual convergence (which hid the R14 rebound). */
static void test_release_backtrack_sweep(void)
{
  const float inertia[] = {0.06f, 0.12f, 0.24f};
  const float gains[] = {1.0f, 1.5f, 2.0f};
  const float filters[] = {0.0f, 0.03f, 0.06f};
  float worst_backtrack = 0.0f;
  for (unsigned t = 0U; t < 3U; ++t)
  for (unsigned g = 0U; g < 3U; ++g)
  for (unsigned f = 0U; f < 3U; ++f)
  for (int sign = -1; sign <= 1; sign += 2)
  for (int translation = 0; translation <= 70; translation += 70)
  {
    YawPlant plant = {170.0f, 0.0f, gains[g], inertia[t], filters[f]};
    float peak, reverse_arc = 0.0f, held;
    unsigned stops;
    reset_manual(170.0f);
    stops = stop_count;
    for (unsigned i = 0U; i < 120U; ++i)
      turn_step(&plant, sign * 80.0f, (int16_t)translation);
    assert(sign * plant.rate > 20.0f);
    peak = sign * plant.yaw;
    for (unsigned i = 0U; i < 200U; ++i)
    {
      turn_step(&plant, 0.0f, (int16_t)translation);
      if (sign * plant.yaw > peak) peak = sign * plant.yaw;
      reverse_arc += fmaxf(0.0f, -sign * plant.rate) * 0.02f;
      assert(fabsf(AttitudeControl_GetSnapshot().output_rpm) <= 60.0f);
      assert(AttitudeControl_GetSnapshot().forward_rpm == translation);
    }
    if (peak - sign * plant.yaw > worst_backtrack)
      worst_backtrack = peak - sign * plant.yaw;
    assert(reverse_arc < 0.5f);
    assert(peak - sign * plant.yaw < 0.5f);
    assert(fabsf(plant.rate) < 1.5f);
    held = AttitudeControl_GetSnapshot().target_deg;
    for (unsigned i = 0U; i < 20U; ++i)
    {
      turn_step(&plant, 0.0f, (int16_t)translation);
      assert(AttitudeControl_GetSnapshot().target_deg == held);
    }
    assert(stop_count == stops); /* No wheel-PID resets during braking. */
  }
  printf("PASS: 108 assumed yaw plants/signs/translation cases; worst release backtrack %.4f deg (not a hardware measurement)\n", worst_backtrack);
  reset_manual(0.0f);
}

static void test_brake_resume_timeout_and_fault(void)
{
  YawPlant plant = {0.0f, 0.0f, 1.5f, 0.12f, 0.03f};
  unsigned stops;
  reset_manual(0.0f);
  stops = stop_count;
  for (unsigned i = 0U; i < 80U; ++i) turn_step(&plant, 80.0f, 30);
  for (unsigned i = 0U; i < 4U; ++i) turn_step(&plant, 0.0f, 30);
  for (unsigned i = 0U; i < 120U; ++i) turn_step(&plant, -80.0f, 30);
  assert(plant.rate < -50.0f && stop_count == stops);
  for (unsigned i = 0U; i < 4U; ++i) turn_step(&plant, 0.0f, 30);
  ChassisTest_Stop();
  assert(!enabled);
  assert(AttitudeControl_GetSnapshot().mode == 0U);
  assert(AttitudeControl_GetSnapshot().output_rpm == 0.0f);

  /* A persistent measured spin must not be silently captured as 'held'. */
  reset_manual(0.0f);
  assert(AttitudeControl_SetManualDrive(80.0f, 0, 0));
  AttitudeControl_Process(20U);
  gyro.rate_dps = 20.0f;
  for (unsigned i = 0U; i < 130U && enabled; ++i)
  {
    tick += 20U;
    assert(AttitudeControl_SetManualDrive(0.0f, 0, 0));
    AttitudeControl_Process(20U);
  }
  assert(!enabled);
  assert(AttitudeControl_GetSnapshot().mode == 0U);

  reset_manual(0.0f);
  assert(AttitudeControl_SetManualDrive(80.0f, 0, 0));
  AttitudeControl_Process(20U);
  gyro.rate_dps = 10.0f;
  assert(AttitudeControl_SetManualDrive(0.0f, 0, 0));
  AttitudeControl_Process(20U);
  /* An explicit angle command exits manual braking, retaining its target
   * and original PD equation rather than capturing the eventual stop. */
  assert(AttitudeControl_SetHeading(45.0f));
  AttitudeControl_Process(20U);
  assert(AttitudeControl_GetSnapshot().target_deg == 45.0f);
  assert(AttitudeControl_GetSnapshot().target_dps == 80.0f);
  gyro.yaw_deg = 40.0f;
  AttitudeControl_Process(20U);
  assert(AttitudeControl_GetSnapshot().target_deg == 45.0f);
  assert(AttitudeControl_GetSnapshot().target_dps == 17.5f);

  reset_manual(0.0f);
  assert(AttitudeControl_SetManualDrive(80.0f, 0, 0));
  AttitudeControl_Process(20U);
  assert(AttitudeControl_SetManualDrive(0.0f, 0, 0));
  AttitudeControl_Process(20U);
  gyro.read_failures = 1U;
  AttitudeControl_Process(20U);
  assert(!enabled && AttitudeControl_GetSnapshot().mode == 0U);
  gyro.read_failures = 0U;
  reset_manual(0.0f);
}

static void test_tracked_heading_plants(void)
{
  const float inertia[] = {0.06f, 0.12f, 0.24f};
  const float gains[] = {1.0f, 1.5f, 2.0f};
  for (unsigned t = 0; t < 3; ++t)
  for (unsigned g = 0; g < 3; ++g)
  for (int sign = -1; sign <= 1; sign += 2)
  {
    YawPlant plant = {170.0f, 0.0f, gains[g], inertia[t], 0.0f};
    float target = sign > 0 ? -100.0f : 80.0f;
    unsigned quiet = 0, stops;
    reset_manual(170.0f);
    stops = stop_count;
    for (unsigned step = 0; step < 750; ++step)
    {
      tick += 20U;
      assert(AttitudeControl_SetTrackedDrive(target, 30.0f, 0.0f));
      AttitudeControl_Process(20U);
      assert(enabled);
      plant.rate += (plant.gain * roundf(AttitudeControl_GetSnapshot().output_rpm) -
                     plant.rate) * (0.02f / plant.tau);
      plant.yaw += plant.rate * 0.02f;
      gyro.yaw_deg = plant.yaw - 360.0f * floorf((plant.yaw + 180.0f) / 360.0f);
      gyro.rate_dps = plant.rate + (step % 2 ? 0.2f : -0.2f);
      if (step >= 650 && AttitudeControl_GetSnapshot().output_rpm == 0.0f) ++quiet;
    }
    assert(fabsf(AttitudeControl_GetSnapshot().error_deg) < 0.8f);
    assert(fabsf(plant.rate) < 1.5f);
    assert(quiet >= 95 && stop_count == stops);
  }
  printf("PASS: 18 assumed fixed-heading plants/signs settle without late wheel hunting (not measured)\n");
  reset_manual(0.0f);
}
int main(void)
{
  memset(response, 0xff, sizeof(response));
  response[1] = 0x73U; response[2] = 0x5aU;
  response[5] = response[6] = response[7] = response[8] = 128U;
  assert(ChassisTest_Init());
  SpeedControl_Init(); AttitudeControl_Init();
  gyro.ready = 1U;
  gyro.yaw_deg = 25.0f;
  Ps2_Init(); Ps2Control_Init();
  for (unsigned i = 0; i < 20U; ++i) poll();
  assert(Ps2_GetSnapshot().connected && Ps2_GetSnapshot().valid);
  assert(Ps2_GetSnapshot().raw_frame[0] == 0xffU);
  assert(sent[0] == 1U && sent[1] == 0x42U && transaction_bytes == 9U);
  assert(!enabled); /* Handshake alone cannot move the vehicle. */
  buttons(0);
  buttons(PS2_BUTTON_START);
  assert(enabled && Ps2Control_IsActive());
  response[8] = 0U; response[7] = 0U; response[5] = 0U;
  buttons(0);
  assert(forward == 15 && left == 15 && rotate == 30);
  buttons(PS2_BUTTON_L2); assert(forward == 30);
  buttons(PS2_BUTTON_R2); assert(forward == 30);
  buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2); assert(forward == 70);
  assert(Ps2Control_GetSnapshot().speed_rpm == 140U);
  response[7] = 128U;
  buttons(PS2_BUTTON_L2 | PS2_BUTTON_R2); assert(forward == 140);
  response[5] = response[7] = response[8] = 128U;
  buttons(0);
  test_continuous_translation_and_hold();
  test_manual_authority_and_noise();
  test_manual_moving_feedback();
  test_release_backtrack_sweep();
  test_brake_resume_timeout_and_fault();
  test_tracked_heading_plants();
  /* IMU failure latches a stop and requires another deliberate START. */
  gyro.read_failures = 1U;
  buttons(0); assert(!enabled && !Ps2Control_IsActive());
  gyro.read_failures = 0U;
  buttons(0); assert(!enabled);
  buttons(PS2_BUTTON_START); assert(enabled);
  buttons(0);
  ChassisTest_Stop();
  buttons(0); assert(!enabled && !Ps2Control_IsActive());
  buttons(PS2_BUTTON_START); assert(enabled);
  buttons(0);
  response[5] = response[7] = response[8] = 128U;
  buttons(PS2_BUTTON_START); /* Toggle off, then keep holding START. */
  assert(!enabled);
  for (unsigned i = 0; i < 10U; ++i) buttons(PS2_BUTTON_START);
  assert(!enabled);
  buttons(0); buttons(PS2_BUTTON_START); assert(enabled);
  buttons(PS2_BUTTON_SELECT | PS2_BUTTON_START); assert(!enabled);
  buttons(PS2_BUTTON_SELECT); buttons(PS2_BUTTON_SELECT | PS2_BUTTON_START);
  assert(!enabled); /* SELECT cannot be bypassed by a second START edge. */
  buttons(0); buttons(PS2_BUTTON_START); assert(enabled);
  memset(response, 0xff, sizeof(response));
  poll(); Ps2Control_Process(); assert(!enabled && stop_count > 0U);
  tick += 200U; Ps2_Process(); assert(!Ps2_GetSnapshot().connected);
  for (unsigned i = 0; i < 60U; ++i) poll();
  assert(Ps2_GetSnapshot().config_attempts >= 2U);
  response[1] = 0x41; response[2] = 0x5a;
  poll();
  assert(!Ps2_GetSnapshot().valid);
  assert(Ps2_GetSnapshot().left_x == 128U);
  response[1] = 0x79; response[5] = response[6] = 128U;
  response[7] = response[8] = 128U;
  for (unsigned i = 0; i < 30U; ++i) poll();
  assert(Ps2_GetSnapshot().valid && transaction_bytes == 21U);
  assert(transactions > 20U);
  /* A debugger can clear trace enable before or during a delay. */
  host_debug.DEMCR = host_dwt.CTRL = 0U;
  poll(); assert(Ps2_GetSnapshot().valid);
  detach_in_delay = 1U;
  poll(); assert(Ps2_GetSnapshot().valid && !detach_in_delay);
  puts("PASS: PS2 wire/reconnect/debug-detach, continuous vector/140 RPM, heading reference/feedforward/release/wrap, independent turn tiers, rotation priority/slew, noise hysteresis, fault stops, START/SELECT");
  return 0;
}
