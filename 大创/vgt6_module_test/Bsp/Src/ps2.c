#include "ps2.h"

#include <string.h>

#include "main.h"
#include "stm32f4xx_hal.h"

#define PS2_POLL_PERIOD_MS       20U
#define PS2_FRAME_BYTES          9U
#define PS2_FRAME_TIMEOUT_MS     150U
#define PS2_MODE_DIGITAL         0x41U
#define PS2_MODE_ANALOG          0x73U
#define PS2_MODE_ANALOG_ALT      0x79U
#define PS2_RECONFIG_PERIOD_MS   1000U

static Ps2Snapshot ps2_state;
static uint32_t poll_tick;
static uint32_t config_tick;
static uint8_t config_step;

static void Ps2_DelayUs(uint32_t microseconds)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = (SystemCoreClock / 1000000U) * microseconds;
  while ((uint32_t)(DWT->CYCCNT - start) < cycles)
  {
    /* pyOCD clears DEMCR on detach. Recover even if this happens inside
     * the delay, so a readback session cannot freeze the control loop. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __NOP();
  }
}

static uint8_t Ps2_TransferByte(uint8_t transmit)
{
  uint8_t receive = 0U;
  uint8_t bit;

  for (bit = 0U; bit < 8U; ++bit)
  {
    HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin,
                      ((transmit & (uint8_t)(1U << bit)) != 0U) ?
                      GPIO_PIN_SET : GPIO_PIN_RESET);
    /* LSB first: change CMD with CLK high, read DAT after the low phase.
     * Match the vendor's 5 us low/high phases and 16 us byte gap. */
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_RESET);
    Ps2_DelayUs(5U);
    if (HAL_GPIO_ReadPin(PS2_DAT_GPIO_Port, PS2_DAT_Pin) != GPIO_PIN_RESET)
    {
      receive |= (uint8_t)(1U << bit);
    }
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
    Ps2_DelayUs(5U);
  }
  HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
  Ps2_DelayUs(16U);
  return receive;
}

static uint8_t Ps2_Exchange(const uint8_t *command, uint8_t *response,
                            uint8_t length)
{
  uint8_t index;
  uint8_t received;

  HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_SEL_GPIO_Port, PS2_SEL_Pin, GPIO_PIN_RESET);
  Ps2_DelayUs(16U);
  for (index = 0U; index < length; ++index)
  {
    received = Ps2_TransferByte(index < PS2_FRAME_BYTES ? command[index] : 0U);
    if (index < PS2_FRAME_BYTES) response[index] = received;
    /* 0x79 includes twelve pressure bytes; clock out the complete frame. */
    if (index == 1U && command[1] == 0x42U &&
        length == PS2_FRAME_BYTES && received == PS2_MODE_ANALOG_ALT)
      length = 21U;
  }
  HAL_GPIO_WritePin(PS2_SEL_GPIO_Port, PS2_SEL_Pin, GPIO_PIN_SET);
  Ps2_DelayUs(16U);
  return 1U;
}

static uint8_t Ps2_ReadFrame(uint8_t *frame)
{
  static const uint8_t command[PS2_FRAME_BYTES] =
      {0x01U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

  (void)Ps2_Exchange(command, frame, PS2_FRAME_BYTES);
  /* 0x01 is the host command, NOT a reply signature. The receiver's first
   * byte is normally idle (0xff); validate mode + 0x5a as in the examples. */
  if ((frame[2] != 0x5AU) ||
      ((frame[1] != PS2_MODE_DIGITAL) &&
       (frame[1] != PS2_MODE_ANALOG) &&
       (frame[1] != PS2_MODE_ANALOG_ALT)))
  {
    return 0U;
  }
  return 1U;
}

static void Ps2_ConfigureStep(void)
{
  static const uint8_t short_poll[5] = {0x01U, 0x42U, 0, 0, 0};
  static const uint8_t enter_config[9] =
      {0x01U, 0x43U, 0, 0x01U, 0, 0, 0, 0, 0};
  static const uint8_t analog_mode[9] =
      {0x01U, 0x44U, 0, 0x01U, 0x03U, 0, 0, 0, 0};
  static const uint8_t exit_config[9] =
      {0x01U, 0x43U, 0, 0, 0x5AU, 0x5AU, 0x5AU, 0x5AU, 0x5AU};
  uint8_t response[9] = {0};

  if (config_step <= 3U)
    (void)Ps2_Exchange(short_poll, response, 5U);
  else if (config_step == 4U)
    (void)Ps2_Exchange(enter_config, response, 9U);
  else if (config_step == 5U)
    (void)Ps2_Exchange(analog_mode, response, 9U);
  else
    (void)Ps2_Exchange(exit_config, response, 9U);
  config_step = config_step == 6U ? 0U : (uint8_t)(config_step + 1U);
}

void Ps2_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  memset(&ps2_state, 0, sizeof(ps2_state));
  ps2_state.left_x = ps2_state.left_y = 128U;
  ps2_state.right_x = ps2_state.right_y = 128U;
  poll_tick = HAL_GetTick() - PS2_POLL_PERIOD_MS;
  config_tick = HAL_GetTick();
  config_step = 1U;
  ps2_state.config_attempts = 1U;
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  gpio.Pin = PS2_CLK_Pin | PS2_CMD_Pin | PS2_SEL_Pin;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &gpio);
  gpio.Pin = PS2_DAT_Pin;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &gpio);

  HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_SEL_GPIO_Port, PS2_SEL_Pin, GPIO_PIN_SET);
}

void Ps2_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t frame[PS2_FRAME_BYTES];
  uint16_t raw_buttons;

  /* Expire the previous frame even while the configuration state machine
   * is running; reconfiguration must not keep a stale connection valid. */
  if (ps2_state.connected &&
      ((now - ps2_state.last_tick) > PS2_FRAME_TIMEOUT_MS))
  {
    ps2_state.connected = 0U;
    ps2_state.valid = 0U;
  }
  if ((now - poll_tick) < PS2_POLL_PERIOD_MS) return;
  poll_tick = now;
  if (config_step != 0U)
  {
    /* One configuration frame per poll after receiver power-up.
     * No HAL_Delay blocks the control loop or emergency-stop processing. */
    if (now - config_tick >= 250U) Ps2_ConfigureStep();
    return;
  }
  if (Ps2_ReadFrame(frame) == 0U)
  {
    memcpy(ps2_state.raw_frame, frame, sizeof(ps2_state.raw_frame));
    ps2_state.valid = 0U;
    if (ps2_state.read_failures != UINT16_MAX)
    {
      ++ps2_state.read_failures;
    }
    if ((now - ps2_state.last_tick) > PS2_FRAME_TIMEOUT_MS)
    {
      ps2_state.connected = 0U;
    }
  }
  else
  {
    memcpy(ps2_state.raw_frame, frame, sizeof(ps2_state.raw_frame));
    raw_buttons = (uint16_t)frame[3] | ((uint16_t)frame[4] << 8U);
    ps2_state.connected = 1U;
    /* Digital 0x41 has no analogue axes. Never use padding as a stick. */
    ps2_state.valid = frame[1] != PS2_MODE_DIGITAL;
    ps2_state.mode = frame[1];
    ps2_state.buttons = (uint16_t)(~raw_buttons);
    ps2_state.right_x = ps2_state.valid ? frame[5] : 128U;
    ps2_state.right_y = ps2_state.valid ? frame[6] : 128U;
    ps2_state.left_x = ps2_state.valid ? frame[7] : 128U;
    ps2_state.left_y = ps2_state.valid ? frame[8] : 128U;
    ps2_state.last_tick = now;
    ps2_state.read_failures = 0U;
  }
  if (!ps2_state.valid && now - config_tick >= PS2_RECONFIG_PERIOD_MS)
  {
    config_tick = now;
    config_step = 1U;
    if (ps2_state.config_attempts != UINT16_MAX) ++ps2_state.config_attempts;
  }
}

Ps2Snapshot Ps2_GetSnapshot(void)
{
  return ps2_state;
}

uint8_t Ps2_IsPressed(uint16_t button)
{
  return ((ps2_state.buttons & button) != 0U) ? 1U : 0U;
}
