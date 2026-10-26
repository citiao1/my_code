#ifndef SERVO_TEST_H
#define SERVO_TEST_H
#include <stdint.h>
typedef struct
{
  uint32_t presses;
  uint32_t commands;
  uint32_t errors;
  uint8_t key_down;
  char last_command;
  char status;
} ServoTestSnapshot;
void ServoTest_Init(void);
void ServoTest_Process(void);
/* SysTick hook: bounded GPIO sampling/event enqueue only, never UART I/O. */
void ServoTest_Sample1ms(void);
char ServoTest_GetStatus(void);
ServoTestSnapshot ServoTest_GetSnapshot(void);
#endif
