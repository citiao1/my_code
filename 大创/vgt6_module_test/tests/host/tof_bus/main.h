#ifndef HOST_TOF_MAIN_H
#define HOST_TOF_MAIN_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
typedef struct { unsigned state, busy; } I2C_HandleTypeDef;
#define HAL_I2C_STATE_READY 1U
#define I2C_FLAG_BUSY 1U
#define I2C_MEMADD_SIZE_16BIT 2U
#define __HAL_I2C_GET_FLAG(h, flag) ((void)(flag), (h)->busy)
uint32_t HAL_GetTick(void);
unsigned HAL_I2C_GetState(I2C_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *handle);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *handle);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *handle);
#endif
