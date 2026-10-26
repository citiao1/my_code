#ifndef TEST_USART_H
#define TEST_USART_H
#include <stdint.h>
typedef struct { uint32_t CR1; } USART_TypeDef;
typedef struct { USART_TypeDef *Instance; } UART_HandleTypeDef;
extern UART_HandleTypeDef huart3;
#define USART_CR1_RE 4U
#define CLEAR_BIT(r,b) ((r) &= ~(b))
#define SET_BIT(r,b) ((r) |= (b))
#define __HAL_UART_FLUSH_DRREGISTER(h) ((void)(h))
#define __HAL_UART_CLEAR_OREFLAG(h) ((void)(h))
#define HAL_OK 0
uint32_t HAL_GetTick(void);
int HAL_UART_Transmit(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
int HAL_UART_Receive(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
#endif
