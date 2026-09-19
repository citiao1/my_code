#ifndef CHASSIS_TEST_H
#define CHASSIS_TEST_H

#include <stdint.h>

#include "motor.h"

typedef struct
{
  uint8_t motor_ready;
  uint8_t armed;
  uint8_t active;
  MotorChannel channel;
  int8_t direction;
  uint8_t duty_percent;
  uint32_t remaining_ms;
} ChassisTestSnapshot;

uint8_t ChassisTest_Init(void);
void ChassisTest_Process(void);
void ChassisTest_Arm(void);
void ChassisTest_Disarm(void);
void ChassisTest_Stop(void);
uint8_t ChassisTest_Jog(MotorChannel channel, int8_t direction,
                        uint8_t duty_percent, uint32_t duration_ms);
uint8_t ChassisTest_Recover(MotorChannel channel);
uint8_t ChassisTest_IsMotorActive(void);
ChassisTestSnapshot ChassisTest_GetSnapshot(void);

#endif /* CHASSIS_TEST_H */
