#include <assert.h>
#include <stdio.h>
#include "main.h"
#include "tof_bus.h"
I2C_HandleTypeDef hi2c1 = {1U, 0U};
static uint32_t tick;
static unsigned transfers, recoveries;
static uint8_t *wire_data;
uint32_t HAL_GetTick(void) { return tick; }
unsigned HAL_I2C_GetState(I2C_HandleTypeDef *handle) { return handle->state; }
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *handle)
{ handle->state = HAL_I2C_STATE_READY; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *handle)
{ handle->state = 0; ++recoveries; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef *h, uint16_t address,
    uint16_t reg, uint16_t width, uint8_t *data, uint16_t count)
{
  assert(h == &hi2c1 && address == 0x52 && reg == 0x87 && width == 2 && count == 1);
  ++transfers; wire_data = data; return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef *h, uint16_t address,
    uint16_t reg, uint16_t width, uint8_t *data, uint16_t count)
{ return HAL_I2C_Mem_Write_IT(h, address, reg, width, data, count); }
int main(void)
{
  uint8_t source = 0x40;
  I2C_HandleTypeDef other = {1, 0};
  TofBus_Init();
  assert(TofBus_Write(0x87, &source, 1));
  source = 0;
  assert(*wire_data == 0x40); /* Stack/caller storage may change after enqueue. */
  assert(!TofBus_Read(0x87, &source, 1));
  HAL_I2C_MemTxCpltCallback(&other);
  assert(TofBus_Poll() == TOF_BUS_BUSY);
  HAL_I2C_MemTxCpltCallback(&hi2c1);
  assert(TofBus_Poll() == TOF_BUS_OK);
  hi2c1.busy = 1;
  assert(!TofBus_Read(0x87, &source, 1) && transfers == 1);
  hi2c1.busy = 0;
  assert(TofBus_Read(0x87, &source, 1));
  tick = 11;
  assert(TofBus_Poll() == TOF_BUS_ERROR && recoveries == 1);
  TofBus_Recover();
  assert(TofBus_Read(0x87, &source, 1));
  HAL_I2C_ErrorCallback(&hi2c1);
  assert(TofBus_Poll() == TOF_BUS_ERROR);
  puts("PASS: ToF IRQ transport buffer ownership, 0x29/16-bit addressing, busy guard and bounded timeout");
  return 0;
}
