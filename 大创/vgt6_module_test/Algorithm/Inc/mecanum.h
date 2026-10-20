#ifndef MECANUM_H
#define MECANUM_H

#include <stdint.h>

typedef struct
{
  /* Physical order fixed by the installed chassis, viewed from above:
   * 1 = right front, 2 = left front, 3 = left rear, 4 = right rear.
   * The numbers increase counter-clockwise from the upper-right wheel. */
  int16_t motor1_right_front;
  int16_t motor2_left_front;
  int16_t motor3_left_rear;
  int16_t motor4_right_rear;
} MecanumWheelTargets;

/* Inputs use vehicle coordinates:
 * forward_rpm > 0 moves the chassis forward,
 * left_rpm > 0 moves it left, and
 * rotate_rpm > 0 turns it counter-clockwise (left). */
void Mecanum_CalculateTargets(int16_t forward_rpm, int16_t left_rpm,
                              int16_t rotate_rpm, uint16_t limit_rpm,
                              MecanumWheelTargets *targets);

#endif
