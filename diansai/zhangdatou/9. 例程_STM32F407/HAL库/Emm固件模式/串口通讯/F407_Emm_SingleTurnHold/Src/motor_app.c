#include "motor_app.h"
#include "motor_uart.h"
#include "Emm_V5.h"

/* 正常使用时必须保持为 0，只有重新标定机械零点时才临时改为 1。 */
#define CALIBRATE_MOTOR_1_ON_BOOT  0
#define CALIBRATE_MOTOR_2_ON_BOOT  0

#define MOTOR_1_ADDRESS       1U
#define MOTOR_2_ADDRESS       2U
#define MOTOR_STEP_PULSES     267UL
#define MOTOR_PULSES_PER_REV  3200UL
#define MOTOR_SPEED_RPM       180U
#define MOTOR_ACCELERATION    40U

#define POWER_UP_DELAY_MS     500UL
#define COMMAND_GAP_MS        100UL
#define HOME_WAIT_MS          3000UL
#define POSITION_POLL_MS      150UL
#define POSITION_TIMEOUT_MS   600UL
#define MOVE_TIMEOUT_MS       10000UL
#define POSITION_TOLERANCE    1.5f

/* 状态号也直接显示在 OLED 上，方便观察程序走到哪一步。 */
#define MOTOR_POWER_UP        0U
#define MOTOR_ENABLE_2        1U
#define MOTOR_HOME_1          2U
#define MOTOR_HOME_2          3U
#define MOTOR_HOMING          4U
#define MOTOR_HOLDING         5U
#define MOTOR_STEP_2          6U
#define MOTOR_STEP_SYNC       7U
#define MOTOR_MOVING          8U
#define MOTOR_CORRECT_2       9U
#define MOTOR_CORRECT_SYNC    10U
#define MOTOR_CORRECTING      11U
#define MOTOR_SET_2           12U
#define MOTOR_SET_SYNC        13U

float motor_angle[2] = {0.0f, 0.0f};
float motor_target_angle[2] = {0.0f, 0.0f};
uint8_t motor_position_valid[2] = {0U, 0U};
uint8_t motor_state = MOTOR_POWER_UP;
uint8_t motor_pending_steps = 0U;
uint32_t motor_cmd_count = 0UL;
uint32_t motor_correction_count = 0UL;
uint32_t motor_error_count = 0UL;

static uint32_t state_time;
static uint32_t last_poll_time;
static uint32_t request_time;
static int32_t target_pulses[2];
static int32_t pending_target_pulses[2];
static uint8_t motor_set_pending;
static uint32_t poll_error_count;
static uint8_t poll_motor;
static uint8_t requested_motor;
static uint8_t waiting_position;
static uint8_t fresh_position;
static uint8_t settle_count;
static uint8_t deviation_count;

void motor_init(void)
{
  uint32_t now = HAL_GetTick();

  motor_angle[0] = 0.0f;
  motor_angle[1] = 0.0f;
  motor_target_angle[0] = 0.0f;
  motor_target_angle[1] = 0.0f;
  motor_position_valid[0] = 0U;
  motor_position_valid[1] = 0U;
  motor_state = MOTOR_POWER_UP;
  motor_pending_steps = 0U;
  motor_cmd_count = 0UL;
  motor_correction_count = 0UL;
  motor_error_count = 0UL;

  state_time = now;
  last_poll_time = now;
  request_time = now;
  target_pulses[0] = 0L;
  target_pulses[1] = 0L;
  pending_target_pulses[0] = 0L;
  pending_target_pulses[1] = 0L;
  motor_set_pending = 0U;
  poll_error_count = 0UL;
  poll_motor = 0U;
  requested_motor = 0U;
  waiting_position = 0U;
  fresh_position = 0U;
  settle_count = 0U;
  deviation_count = 0U;
}

void motor_add_30(void)
{
  if (motor_pending_steps < 99U)
  {
    ++motor_pending_steps;
  }
}

uint8_t motor_set_angles(int16_t motor_1_angle_10, int16_t motor_2_angle_10)
{
  if ((motor_1_angle_10 < -3600) || (motor_1_angle_10 > 3600) ||
      (motor_2_angle_10 < -3600) || (motor_2_angle_10 > 3600))
  {
    return 0U;
  }

  /* 蓝牙角度使用 0.1 度为单位，换算为电机绝对位置脉冲。 */
  if (motor_1_angle_10 >= 0)
  {
    pending_target_pulses[0] =
      ((int32_t)motor_1_angle_10 * (int32_t)MOTOR_PULSES_PER_REV + 1800L) /
      3600L;
  }
  else
  {
    pending_target_pulses[0] =
      -((-(int32_t)motor_1_angle_10 * (int32_t)MOTOR_PULSES_PER_REV +
         1800L) / 3600L);
  }

  if (motor_2_angle_10 >= 0)
  {
    pending_target_pulses[1] =
      ((int32_t)motor_2_angle_10 * (int32_t)MOTOR_PULSES_PER_REV + 1800L) /
      3600L;
  }
  else
  {
    pending_target_pulses[1] =
      -((-(int32_t)motor_2_angle_10 * (int32_t)MOTOR_PULSES_PER_REV +
         1800L) / 3600L);
  }

  motor_set_pending = 1U;
  return 1U;
}

void motor_pro(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t index;
  uint8_t on_target;
  float error_1;
  float error_2;

  motor_error_count = poll_error_count + uart_error_count;

  /* 处理 uart_pro() 已经解析好的位置数据。 */
  if (waiting_position != 0U)
  {
    index = requested_motor;
    if (uart_position_ready[index] != 0U)
    {
      uart_position_ready[index] = 0U;
      motor_angle[index] = uart_angle[index];
      motor_position_valid[index] = 1U;
      fresh_position |= (uint8_t)(1U << index);
      waiting_position = 0U;
      poll_motor = (uint8_t)((index + 1U) % 2U);

      /* 两台电机都更新过一次后，再判断是否到位或被外力拨动。 */
      if (fresh_position == 0x03U)
      {
        fresh_position = 0U;
        error_1 = motor_angle[0] - motor_target_angle[0];
        error_2 = motor_angle[1] - motor_target_angle[1];
        if (error_1 < 0.0f)
        {
          error_1 = -error_1;
        }
        if (error_2 < 0.0f)
        {
          error_2 = -error_2;
        }
        on_target = ((motor_position_valid[0] != 0U) &&
                     (motor_position_valid[1] != 0U) &&
                     (error_1 <= POSITION_TOLERANCE) &&
                     (error_2 <= POSITION_TOLERANCE)) ? 1U : 0U;

        if ((motor_state == MOTOR_MOVING) ||
            (motor_state == MOTOR_CORRECTING))
        {
          if (on_target != 0U)
          {
            ++settle_count;
            if (settle_count >= 2U)
            {
              motor_state = MOTOR_HOLDING;
              state_time = now;
              last_poll_time = now;
              settle_count = 0U;
              deviation_count = 0U;
            }
          }
          else
          {
            settle_count = 0U;
          }
        }
        else if (motor_state == MOTOR_HOLDING)
        {
          if (on_target == 0U)
          {
            ++deviation_count;
            if (deviation_count >= 3U)
            {
              /* 回正用累计绝对目标 raF=1，不会额外再加 30 度。 */
               Emm_V5_Pos_Control(MOTOR_1_ADDRESS,
                                  (target_pulses[0] < 0L) ? 1U : 0U,
                                  MOTOR_SPEED_RPM, MOTOR_ACCELERATION,
                                  (uint32_t)((target_pulses[0] < 0L) ?
                                  -target_pulses[0] : target_pulses[0]),
                                  1U, true);
              motor_state = MOTOR_CORRECT_2;
              state_time = now;
              deviation_count = 0U;
            }
          }
          else
          {
            deviation_count = 0U;
          }
        }
      }
    }
    else if ((uint32_t)(now - request_time) >= POSITION_TIMEOUT_MS)
    {
      waiting_position = 0U;
      poll_motor = (uint8_t)((index + 1U) % 2U);
      ++poll_error_count;
    }
  }

  switch (motor_state)
  {
    case MOTOR_POWER_UP:
      if ((uint32_t)(now - state_time) >= POWER_UP_DELAY_MS)
      {
        Emm_V5_En_Control(MOTOR_1_ADDRESS, true, false);
        motor_state = MOTOR_ENABLE_2;
        state_time = now;
      }
      break;

    case MOTOR_ENABLE_2:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_En_Control(MOTOR_2_ADDRESS, true, false);
        motor_state = MOTOR_HOME_1;
        state_time = now;
      }
      break;

    case MOTOR_HOME_1:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
#if CALIBRATE_MOTOR_1_ON_BOOT
        Emm_V5_Origin_Set_O(MOTOR_1_ADDRESS, true);
#else
        Emm_V5_Origin_Trigger_Return(MOTOR_1_ADDRESS, 0U, false);
#endif
        motor_state = MOTOR_HOME_2;
        state_time = now;
      }
      break;

    case MOTOR_HOME_2:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
#if CALIBRATE_MOTOR_2_ON_BOOT
        Emm_V5_Origin_Set_O(MOTOR_2_ADDRESS, true);
#else
        Emm_V5_Origin_Trigger_Return(MOTOR_2_ADDRESS, 0U, false);
#endif
        motor_state = MOTOR_HOMING;
        state_time = now;
      }
      break;

    case MOTOR_HOMING:
      if ((uint32_t)(now - state_time) >= HOME_WAIT_MS)
      {
        motor_state = MOTOR_HOLDING;
        state_time = now;
        last_poll_time = now;
        fresh_position = 0U;
      }
      break;

    case MOTOR_HOLDING:
      if ((motor_set_pending != 0U) && (waiting_position == 0U))
      {
        target_pulses[0] = pending_target_pulses[0];
        target_pulses[1] = pending_target_pulses[1];
        motor_target_angle[0] = ((float)target_pulses[0] * 360.0f) /
                                (float)MOTOR_PULSES_PER_REV;
        motor_target_angle[1] = ((float)target_pulses[1] * 360.0f) /
                                (float)MOTOR_PULSES_PER_REV;
        motor_set_pending = 0U;

        Emm_V5_Pos_Control(MOTOR_1_ADDRESS,
                           (target_pulses[0] < 0L) ? 1U : 0U,
                           MOTOR_SPEED_RPM, MOTOR_ACCELERATION,
                           (uint32_t)((target_pulses[0] < 0L) ?
                           -target_pulses[0] : target_pulses[0]),
                           1U, true);
        motor_state = MOTOR_SET_2;
        state_time = now;
      }
      else if ((motor_pending_steps != 0U) && (waiting_position == 0U))
      {
        --motor_pending_steps;
        target_pulses[0] += (int32_t)MOTOR_STEP_PULSES;
        target_pulses[1] += (int32_t)MOTOR_STEP_PULSES;
        motor_target_angle[0] = ((float)target_pulses[0] * 360.0f) /
                                (float)MOTOR_PULSES_PER_REV;
        motor_target_angle[1] = ((float)target_pulses[1] * 360.0f) /
                                (float)MOTOR_PULSES_PER_REV;

        /* raF=0：每按一次都相对上一次目标继续顺时针增加约 30 度。 */
        Emm_V5_Pos_Control(MOTOR_1_ADDRESS, 0U, MOTOR_SPEED_RPM,
                           MOTOR_ACCELERATION, MOTOR_STEP_PULSES, 0U, true);
        motor_state = MOTOR_STEP_2;
        state_time = now;
      }
      break;

    case MOTOR_SET_2:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_Pos_Control(MOTOR_2_ADDRESS,
                           (target_pulses[1] < 0L) ? 1U : 0U,
                           MOTOR_SPEED_RPM, MOTOR_ACCELERATION,
                           (uint32_t)((target_pulses[1] < 0L) ?
                           -target_pulses[1] : target_pulses[1]),
                           1U, true);
        motor_state = MOTOR_SET_SYNC;
        state_time = now;
      }
      break;

    case MOTOR_SET_SYNC:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_Synchronous_motion(0U);
        ++motor_cmd_count;
        motor_state = MOTOR_MOVING;
        state_time = now;
        last_poll_time = now;
        fresh_position = 0U;
        settle_count = 0U;
      }
      break;

    case MOTOR_STEP_2:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_Pos_Control(MOTOR_2_ADDRESS, 0U, MOTOR_SPEED_RPM,
                           MOTOR_ACCELERATION, MOTOR_STEP_PULSES, 0U, true);
        motor_state = MOTOR_STEP_SYNC;
        state_time = now;
      }
      break;

    case MOTOR_STEP_SYNC:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_Synchronous_motion(0U);
        ++motor_cmd_count;
        motor_state = MOTOR_MOVING;
        state_time = now;
        last_poll_time = now;
        fresh_position = 0U;
        settle_count = 0U;
      }
      break;

    case MOTOR_MOVING:
      if ((uint32_t)(now - state_time) >= MOVE_TIMEOUT_MS)
      {
        ++poll_error_count;
        motor_state = MOTOR_HOLDING;
        state_time = now;
        last_poll_time = now;
        waiting_position = 0U;
      }
      break;

    case MOTOR_CORRECT_2:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_Pos_Control(MOTOR_2_ADDRESS,
                           (target_pulses[1] < 0L) ? 1U : 0U,
                           MOTOR_SPEED_RPM, MOTOR_ACCELERATION,
                           (uint32_t)((target_pulses[1] < 0L) ?
                           -target_pulses[1] : target_pulses[1]),
                           1U, true);
        motor_state = MOTOR_CORRECT_SYNC;
        state_time = now;
      }
      break;

    case MOTOR_CORRECT_SYNC:
      if ((uint32_t)(now - state_time) >= COMMAND_GAP_MS)
      {
        Emm_V5_Synchronous_motion(0U);
        ++motor_correction_count;
        motor_state = MOTOR_CORRECTING;
        state_time = now;
        last_poll_time = now;
        fresh_position = 0U;
        settle_count = 0U;
      }
      break;

    case MOTOR_CORRECTING:
      if ((uint32_t)(now - state_time) >= MOVE_TIMEOUT_MS)
      {
        ++poll_error_count;
        motor_state = MOTOR_HOLDING;
        state_time = now;
        last_poll_time = now;
        waiting_position = 0U;
      }
      break;

    default:
      motor_init();
      break;
  }

  /* 保持、运动和回正阶段轮流读取 1、2 号电机的位置。 */
  if (((motor_state == MOTOR_HOLDING) ||
       (motor_state == MOTOR_MOVING) ||
       (motor_state == MOTOR_CORRECTING)) &&
      (waiting_position == 0U) &&
      ((uint32_t)(now - last_poll_time) >= POSITION_POLL_MS))
  {
    index = poll_motor;
    uart_position_ready[index] = 0U;
    Emm_V5_Read_Sys_Params((index == 0U) ? MOTOR_1_ADDRESS : MOTOR_2_ADDRESS,
                           S_CPOS);
    requested_motor = index;
    request_time = now;
    last_poll_time = now;
    waiting_position = 1U;
  }
}
