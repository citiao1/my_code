#ifndef MECANUM_H
#define MECANUM_H

#include <stdint.h>

typedef struct
{
  int16_t right_rear;
  int16_t left_rear;
  int16_t right_front;
  int16_t left_front;
} MecanumWheelTargets;

void Mecanum_CalculateTargets(int16_t forward_rpm, int16_t left_rpm,
                              int16_t rotate_rpm, uint16_t limit_rpm,
                              MecanumWheelTargets *targets);

#endif
