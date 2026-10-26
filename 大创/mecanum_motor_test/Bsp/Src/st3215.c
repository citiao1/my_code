#include "st3215.h"
#include "usart.h"
#include <string.h>

#define SERVO_ID 1U
#define IO_TIMEOUT_MS 5U
#define TEST_SPEED 500U

/* USART3 (PC10 TX / PC11 RX) belongs exclusively to this driver, through the TTL bus adapter.
 * Transactions run in the main loop. No DMA RX or Bluetooth parser is active. */
static uint8_t Transfer(uint8_t instruction, const uint8_t *params,
                        uint8_t count, uint8_t *result, uint8_t size)
{
  uint8_t tx[16], rx[16], byte, used = 0U, i, sum;
  uint32_t start;
  tx[0] = 0xFFU; tx[1] = 0xFFU; tx[2] = SERVO_ID;
  tx[3] = count + 2U; tx[4] = instruction;
  memcpy(&tx[5], params, count);
  sum = 0U;
  for (i = 2U; i < count + 5U; ++i) sum += tx[i];
  tx[count + 5U] = (uint8_t)~sum;
  __HAL_UART_FLUSH_DRREGISTER(&huart3);
  __HAL_UART_CLEAR_OREFLAG(&huart3);
  /* Ignore local bus echo while transmitting. HAL waits for TC before return. */
  CLEAR_BIT(huart3.Instance->CR1, USART_CR1_RE);
  i = HAL_UART_Transmit(&huart3, tx, count + 6U, IO_TIMEOUT_MS) == HAL_OK;
  SET_BIT(huart3.Instance->CR1, USART_CR1_RE);
  if (!i) return 0U;
  start = HAL_GetTick();
  while ((HAL_GetTick() - start) < IO_TIMEOUT_MS)
  {
    if (HAL_UART_Receive(&huart3, &byte, 1U, 1U) != HAL_OK) continue;
    if (used == sizeof(rx)) { memmove(rx, rx + 1, sizeof(rx) - 1U); --used; }
    rx[used++] = byte;
    if (used < size + 6U) continue;
    /* Scan a bounded sliding window: reject adapter echo and bad checksums. */
    i = used - size - 6U;
    if ((rx[i] != 0xFFU) || (rx[i+1U] != 0xFFU) ||
        (rx[i+2U] != SERVO_ID) || (rx[i+3U] != size+2U) ||
        (rx[i+4U] != 0U)) continue;
    sum = 0U;
    for (byte = i+2U; byte < used; ++byte) sum += rx[byte];
    if (sum != 0xFFU) continue;
    if (size != 0U) memcpy(result, &rx[i+5U], size);
    return 1U;
  }
  return 0U;
}

static uint8_t Write(uint8_t address, uint8_t value)
{
  uint8_t params[2] = {address, value};
  return Transfer(3U, params, 2U, 0, 0U);
}

static uint8_t Read(uint8_t address, uint8_t *data, uint8_t count)
{
  uint8_t params[2] = {address, count};
  return Transfer(2U, params, 2U, data, count);
}

static uint8_t Mode(uint8_t mode)
{
  uint8_t actual;
  /* Lock=1 keeps mode changes volatile; never unlock/write EEPROM. */
  return Write(0x37U, 1U) && Write(0x21U, mode) &&
         Read(0x21U, &actual, 1U) && (actual == mode);
}

static uint8_t Goal(uint16_t position, uint16_t speed)
{
  uint8_t params[8] = {0x29U, 50U, 0U, 0U, 0U, 0U, 0U, 0U};
  params[2] = (uint8_t)position; params[3] = (uint8_t)(position >> 8);
  params[6] = (uint8_t)speed; params[7] = (uint8_t)(speed >> 8);
  return Transfer(3U, params, sizeof(params), 0, 0U);
}

uint8_t St3215_Stop(void)
{
  /* Torque off stops even when phase bit 3 makes speed zero mean maximum. */
  return Write(0x28U, 0U);
}

uint8_t St3215_Check(void)
{
  uint8_t status;
  return Read(0x41U, &status, 1U) && (status == 0U);
}

uint8_t St3215_Init(void)
{
  uint8_t phase;
  /* Step mode requires both angle limits zero; all changes stay in RAM. */
  return St3215_Stop() && Write(0x37U, 1U) &&
         Write(0x09U, 0U) && Write(0x0AU, 0U) &&
         Write(0x0BU, 0U) && Write(0x0CU, 0U) &&
         Read(0x12U, &phase, 1U) && Write(0x12U, phase | 4U) && Mode(3U);
}

uint8_t St3215_Step(int8_t direction)
{
  uint16_t steps = 341U;
  if (!St3215_Stop() || !Mode(3U)) return 0U;
  /* Relative step mode avoids reversing 330 degrees at the encoder wrap.
   * 4096/12 rounds to 341 counts (29.9707 degrees); BIT15 is direction. */
  if (direction < 0) steps |= 0x8000U;
  return Goal(steps, TEST_SPEED) && Write(0x28U, 1U);
}

uint8_t St3215_Run(void)
{
  return St3215_Stop() && Mode(1U) && Goal(0U, TEST_SPEED) && Write(0x28U, 1U);
}
