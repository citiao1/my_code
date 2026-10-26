#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#define USER_KEY_GPIO_Port 0
#define USER_KEY_Pin 0
#define GPIO_PIN_RESET 0
uint32_t HAL_GetTick(void);
int HAL_GPIO_ReadPin(int port, int pin);
#endif
