#include "tof_bus.h"
#include <string.h>
#include "main.h"

extern I2C_HandleTypeDef hi2c1;
#define TOF_I2C_ADDRESS (0x29U << 1)
#define TOF_BUS_TIMEOUT_MS 10U
static uint8_t tx_buffer[16];
static volatile TofBusResult result;
static uint32_t started_tick;

void TofBus_Init(void) { result = TOF_BUS_OK; }

void TofBus_Recover(void)
{
  /* Main-loop only. Reset this dedicated peripheral, never the motor timers.
   * HAL init/deinit use the generated MSP pin/IRQ configuration. */
  (void)HAL_I2C_DeInit(&hi2c1);
  (void)HAL_I2C_Init(&hi2c1);
  result = TOF_BUS_OK;
}

static uint8_t begin(void)
{
  /* The HAL _IT API waits synchronously for BUSY before enabling IRQs.
   * Reject an already-stuck bus before entering that wait. */
  if (result == TOF_BUS_BUSY ||
      HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY ||
      __HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY))
    return 0U;
  started_tick = HAL_GetTick();
  result = TOF_BUS_BUSY;
  return 1U;
}

uint8_t TofBus_Read(uint16_t reg, uint8_t *data, uint16_t count)
{
  if (!data || !count || count > 16U || !begin()) return 0U;
  if (HAL_I2C_Mem_Read_IT(&hi2c1, TOF_I2C_ADDRESS, reg,
                         I2C_MEMADD_SIZE_16BIT, data, count) != HAL_OK)
  {
    result = TOF_BUS_ERROR;
    return 0U;
  }
  return 1U;
}

uint8_t TofBus_Write(uint16_t reg, const uint8_t *data, uint16_t count)
{
  if (!data || !count || count > sizeof(tx_buffer) || !begin()) return 0U;
  memcpy(tx_buffer, data, count);
  if (HAL_I2C_Mem_Write_IT(&hi2c1, TOF_I2C_ADDRESS, reg,
                          I2C_MEMADD_SIZE_16BIT, tx_buffer, count) != HAL_OK)
  {
    result = TOF_BUS_ERROR;
    return 0U;
  }
  return 1U;
}

TofBusResult TofBus_Poll(void)
{
  if (result == TOF_BUS_BUSY &&
      HAL_GetTick() - started_tick > TOF_BUS_TIMEOUT_MS)
  {
    TofBus_Recover();
    result = TOF_BUS_ERROR;
  }
  return result;
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *handle)
{
  if (handle == &hi2c1 && result == TOF_BUS_BUSY) result = TOF_BUS_OK;
}
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *handle)
{
  if (handle == &hi2c1 && result == TOF_BUS_BUSY) result = TOF_BUS_OK;
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *handle)
{
  if (handle == &hi2c1 && result == TOF_BUS_BUSY) result = TOF_BUS_ERROR;
}
