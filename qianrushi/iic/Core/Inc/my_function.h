#ifndef _MY_FUNCTION_H
#define _MY_FUNCTION_H
#include "stm32f1xx_hal.h"
#include "main.h"
#define     BUFFERSIZE      256
#define     REC_LENGTH      1
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;
typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;
typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;
extern unsigned char Rx_Buff[BUFFERSIZE];
extern unsigned char Rx_flg;
extern unsigned int  Rx_cnt;
extern uint8_t aRX;
extern uint8_t receiveData[1000];
extern u32 uwTick_Key;
extern u8 KeyValue;
extern u8 KeyState;

typedef const uint32_t uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */
#define ON "ON"
#define OFF "OFF"
#define B1 	HAL_GPIO_ReadPin(K1_GPIO_Port,K1_Pin)
#define B2 	HAL_GPIO_ReadPin(K2_GPIO_Port,K2_Pin)

#define KEY_STATE_0 0
#define KEY_STATE_1 1
#define KEY_STATE_2 2
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void Blue_LED(const char *state);
void Red_LED(const char *state);
void Green_LED(const char *state);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void Key_Pro(void);
void Rx_Process(void);
uint8_t key_scanf(void);


#endif
