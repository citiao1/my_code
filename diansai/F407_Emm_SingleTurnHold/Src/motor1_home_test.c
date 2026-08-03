#include "motor1_home_test.h"
#include "Emm_V5.h"

#define MOTOR1_HOME_TEST_ADDRESS     1U
#define HOME_TEST_POWERUP_DELAY_MS   500UL
#define HOME_TEST_COMMAND_GAP_MS     100UL

typedef enum
{
  HOME_TEST_WAIT_POWERUP = 0U,
  HOME_TEST_WAIT_ENABLE,
  HOME_TEST_DONE
} Motor1HomeTestState_t;

static Motor1HomeTestState_t home_test_state;
static uint32_t home_test_tick;

void motor1_home_test_init(void)
{
  home_test_state = HOME_TEST_WAIT_POWERUP;
  home_test_tick = HAL_GetTick();
}

void motor1_home_test_run(void)
{
  uint32_t now = HAL_GetTick();

  switch (home_test_state)
  {
    case HOME_TEST_WAIT_POWERUP:
      if ((uint32_t)(now - home_test_tick) >= HOME_TEST_POWERUP_DELAY_MS)
      {
        /* 仅使能地址 1。 */
        Emm_V5_En_Control(MOTOR1_HOME_TEST_ADDRESS, true, false);
        home_test_state = HOME_TEST_WAIT_ENABLE;
        home_test_tick = now;
      }
      break;

    case HOME_TEST_WAIT_ENABLE:
      if ((uint32_t)(now - home_test_tick) >= HOME_TEST_COMMAND_GAP_MS)
      {
        /* 按最近路径回到已经保存的单圈零点，不修改零点。 */
        Emm_V5_Origin_Trigger_Return(MOTOR1_HOME_TEST_ADDRESS, 0U, false);
        home_test_state = HOME_TEST_DONE;
      }
      break;

    case HOME_TEST_DONE:
    default:
      /* 回零指令只发一次，电机驱动器自行完成后续运动。 */
      break;
  }
}
