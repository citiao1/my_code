#ifndef SERVO_TEST_H
#define SERVO_TEST_H
#include <stdint.h>
void ServoTest_Init(void);
void ServoTest_Process(void);
/* SysTick hook: bounded GPIO sampling/event enqueue only, never UART I/O. */
void ServoTest_Sample1ms(void);
char ServoTest_GetStatus(void);
#endif
