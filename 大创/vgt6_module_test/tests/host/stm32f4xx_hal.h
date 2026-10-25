#ifndef HOST_STM32_HAL_H
#define HOST_STM32_HAL_H
#include <stdint.h>
typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { int unused; } DMA_HandleTypeDef;
typedef struct { uint32_t BRR; } USART_TypeDef;
typedef struct { uint32_t BaudRate; } UART_InitTypeDef;
typedef struct {
  DMA_HandleTypeDef *hdmarx;
  UART_InitTypeDef Init;
  USART_TypeDef *Instance;
  uint32_t ErrorCode;
} UART_HandleTypeDef;
#define HAL_UART_ERROR_PE 1U
#define HAL_UART_ERROR_NE 2U
#define HAL_UART_ERROR_FE 4U
#define HAL_UART_ERROR_ORE 8U
#define HAL_UART_ERROR_DMA 16U
#define DMA_IT_HT 1U
#define __HAL_DMA_DISABLE_IT(handle, flag) ((void)(handle), (void)(flag))
uint32_t HAL_GetTick(void);
uint32_t HAL_RCC_GetPCLK1Freq(void);
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *);
#ifndef __get_PRIMASK
#define __get_PRIMASK() 0U
#endif
#ifndef __disable_irq
#define __disable_irq() ((void)0)
#endif
#ifndef __set_PRIMASK
#define __set_PRIMASK(mask) ((void)(mask))
#endif
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *, uint16_t);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *);
#endif
