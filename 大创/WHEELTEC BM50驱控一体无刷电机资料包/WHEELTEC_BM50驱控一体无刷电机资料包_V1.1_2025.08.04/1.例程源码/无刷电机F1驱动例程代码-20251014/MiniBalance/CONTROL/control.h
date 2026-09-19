#ifndef __CONTROL_H
#define __CONTROL_H
#include "sys.h"
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/
extern float Accel_Y,Accel_Z,Accel_X,Gyrox,Gyroy,Gyroz;
#define PI 3.14159265
#define ZHONGZHI 0 
#define DIFFERENCE 100
void Set_Pwm(int motor_a);
u8 Turn_Off( int voltage);
u32 myabs(long int a);
int Incremental_PI(int Encoder,int Target);
float PWM_Limit(float IN,int max,int min);
#endif
