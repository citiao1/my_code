#include "motor1_calibration.h"
#include "Emm_V5.h"

#define MOTOR1_CALIBRATION_ADDRESS  1U
#define CALIBRATION_POWERUP_DELAY_MS 500UL
#define CALIBRATION_COMMAND_GAP_MS  100UL

typedef enum
{
  CALIBRATION_WAIT_POWERUP = 0U,
  CALIBRATION_WAIT_ENABLE,
  CALIBRATION_DONE
} Motor1CalibrationState_t;

static Motor1CalibrationState_t calibration_state;
static uint32_t calibration_state_tick;

void motor1_calibration_init(void)
{
  calibration_state = CALIBRATION_WAIT_POWERUP;
  calibration_state_tick = HAL_GetTick();
}

void motor1_calibration_run(void)
{
  uint32_t now = HAL_GetTick();

  switch (calibration_state)
  {
    case CALIBRATION_WAIT_POWERUP:
      if ((uint32_t)(now - calibration_state_tick) >=
          CALIBRATION_POWERUP_DELAY_MS)
      {
        /* 只使能地址 1，避免校准程序误操作第二个电机。 */
        Emm_V5_En_Control(MOTOR1_CALIBRATION_ADDRESS, true, false);
        calibration_state = CALIBRATION_WAIT_ENABLE;
        calibration_state_tick = now;
      }
      break;

    case CALIBRATION_WAIT_ENABLE:
      if ((uint32_t)(now - calibration_state_tick) >=
          CALIBRATION_COMMAND_GAP_MS)
      {
        /* 此时机械轴必须已经手动放在真实零点。 */
        Emm_V5_Origin_Set_O(MOTOR1_CALIBRATION_ADDRESS, true);
        calibration_state = CALIBRATION_DONE;
        calibration_state_tick = now;
      }
      break;

    case CALIBRATION_DONE:
    default:
      /* 校准命令只发送一次，完成后不再重复写入零点。 */
      break;
  }
}
