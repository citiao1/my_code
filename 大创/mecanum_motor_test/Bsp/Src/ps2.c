#include "ps2.h"

#include "main.h"
#include "stm32f4xx_hal.h"

#define PS2_HALF_CLOCK_US       5U
#define PS2_INTER_BYTE_US      16U
#define PS2_FRAME_SIZE          9U

static void Ps2_DelayUs(uint32_t microseconds)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = (SystemCoreClock / 1000000U) * microseconds;

  while ((DWT->CYCCNT - start) < cycles)
  {
  }
}

static uint8_t Ps2_ExchangeByte(uint8_t command)
{
  uint8_t response = 0U;
  uint8_t mask;

  for (mask = 1U; mask != 0U; mask <<= 1U)
  {
    HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin,
                      ((command & mask) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
    Ps2_DelayUs(PS2_HALF_CLOCK_US);
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_RESET);
    Ps2_DelayUs(PS2_HALF_CLOCK_US);
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);

    if (HAL_GPIO_ReadPin(PS2_DAT_GPIO_Port, PS2_DAT_Pin) == GPIO_PIN_SET)
    {
      response |= mask;
    }
  }

  Ps2_DelayUs(PS2_INTER_BYTE_US);
  return response;
}

static void Ps2_Transfer(const uint8_t *command, uint8_t *response, uint8_t length)
{
  uint8_t index;

  HAL_GPIO_WritePin(PS2_CS_GPIO_Port, PS2_CS_Pin, GPIO_PIN_RESET);
  Ps2_DelayUs(PS2_HALF_CLOCK_US);

  for (index = 0U; index < length; ++index)
  {
    uint8_t value = Ps2_ExchangeByte(command[index]);
    if (response != 0)
    {
      response[index] = value;
    }
  }

  HAL_GPIO_WritePin(PS2_CS_GPIO_Port, PS2_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
}

static void Ps2_SendConfiguration(const uint8_t command[PS2_FRAME_SIZE])
{
  Ps2_Transfer(command, 0, PS2_FRAME_SIZE);
  HAL_Delay(10U);
}

void Ps2_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_CS_GPIO_Port, PS2_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);

  HAL_Delay(500U);
  Ps2_ConfigureAnalog();
}

void Ps2_ConfigureAnalog(void)
{
  static const uint8_t poll_command[PS2_FRAME_SIZE] = {
    0x01U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
  };
  static const uint8_t enter_config[PS2_FRAME_SIZE] = {
    0x01U, 0x43U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
  };
  static const uint8_t set_analog[PS2_FRAME_SIZE] = {
    0x01U, 0x44U, 0x00U, 0x01U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U
  };
  static const uint8_t exit_config[PS2_FRAME_SIZE] = {
    0x01U, 0x43U, 0x00U, 0x00U, 0x5AU, 0x5AU, 0x5AU, 0x5AU, 0x5AU
  };
  uint8_t index;

  for (index = 0U; index < 3U; ++index)
  {
    Ps2_SendConfiguration(poll_command);
  }
  Ps2_SendConfiguration(enter_config);
  Ps2_SendConfiguration(set_analog);
  Ps2_SendConfiguration(exit_config);
}

uint8_t Ps2_Poll(Ps2State *state)
{
  static const uint8_t poll_command[PS2_FRAME_SIZE] = {
    0x01U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
  };
  uint8_t response[PS2_FRAME_SIZE] = {0U};
  uint16_t raw_buttons;

  if (state == 0)
  {
    return 0U;
  }

  state->connected = 0U;
  state->analog_mode = 0U;
  Ps2_Transfer(poll_command, response, PS2_FRAME_SIZE);

  if (((response[1] != 0x41U) && (response[1] != 0x73U) &&
       (response[1] != 0x79U)) || (response[2] != 0x5AU))
  {
    return 0U;
  }

  state->connected = 1U;
  state->analog_mode = (uint8_t)((response[1] == 0x73U) ||
                                 (response[1] == 0x79U));
  raw_buttons = (uint16_t)response[3] | ((uint16_t)response[4] << 8U);
  state->buttons = (uint16_t)(~raw_buttons);
  state->right_x = response[5];
  state->right_y = response[6];
  state->left_x = response[7];
  state->left_y = response[8];
  return 1U;
}
