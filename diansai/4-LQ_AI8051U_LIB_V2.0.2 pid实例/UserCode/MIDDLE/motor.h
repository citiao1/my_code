#ifndef __MOTOR_H_
#define __MOTOR_H_

#include "LQ_Encoder.h"
#include "LQ_MotorServo.h"
#include "pid.h"
#include "DEF.h"

extern int16 _encoder_L;
extern int16 _encoder_R;
extern int16 left_pwm;
extern int16 right_pwm;
extern int16 vofa_target_speed;
extern int16 vofa_current_speed;
extern int16 vofa_out_pwm;
extern int16 stop_flag;
extern double car_distance;
extern int set_distance_calculation;
extern double global_distance;

// 负压风扇全局控制变量
extern target_fan_pwm ;
extern int16 current_fan_pwm ; 
void Motor_Control(int16 Left_Target_Speed, int16 Right_Target_Speed);
void Fan_Smooth_Task(void);
void Clear_Car_Distance(void);
void Car_Distance_Calculate(void);
void Clear_Global_Distance(void);
#endif