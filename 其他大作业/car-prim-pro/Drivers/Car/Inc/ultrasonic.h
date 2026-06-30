#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

extern volatile float distance;
extern volatile uint32_t g_distance_time;
extern volatile float g_distance ;

extern float D_front;
extern float D_left;
extern float D_right;

float Ultrasonic_GetDistance_NonBlocking(void);
void Ultrasonic_Trig(void);
float Ultrasonic_GetDistance_Filtered(void);

#endif