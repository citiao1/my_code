#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

extern volatile float distance1,distance2,distance3;
extern volatile uint32_t g_distance_time_1,g_distance_time_2,g_distance_time_3;
extern volatile float g_distance ;

extern volatile float dist1;
extern volatile float dist2;
extern volatile float dist3;

float Ultrasonic_GetDistance_NonBlocking(void);
void Ultrasonic_Trig(void);
void Ultrasonic_Trig_Pin(GPIO_TypeDef* GPIOx ,uint16_t Trig_PIN);
float Ultrasonic1_GetDistance_Filtered(void);
float Ultrasonic2_GetDistance_Filtered(void);
float Ultrasonic3_GetDistance_Filtered(void);

#endif