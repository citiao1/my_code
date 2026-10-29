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
uint8_t ServoTest_Stop(void);
uint8_t ServoTest_Move(int16_t angle_d10, uint16_t speed);
uint8_t ServoTest_Automatic(int16_t angle_d10, uint16_t speed);
uint8_t ServoTest_Continuous(int8_t direction, uint16_t speed);
void ServoTest_KeepAlive(void);
const char *ServoTest_GetModeText(void);
#endif
