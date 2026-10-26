#ifndef ST3215_H
#define ST3215_H
#include <stdint.h>
uint8_t St3215_Init(void);
uint8_t St3215_Start(void);
uint8_t St3215_GetMoving(uint8_t *moving);
uint8_t St3215_Step(int8_t direction);
uint8_t St3215_Run(void);
uint8_t St3215_Stop(void);
uint8_t St3215_Check(void);
#endif
