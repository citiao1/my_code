#ifndef _mycan__H__
#define _mycan__H__

#include "main.h"
#define ECODE_K   360.0f / 8191.0f
#define SPEED_SMOOTH_COEF 0.85f
#define RPM_2_ANGLE_PER_SEC 6.0f
typedef struct
{
uint16_t angle;
int16_t speed;
int16_t current;
uint8_t temp;
int16_t motor_voltage; 
uint16_t id;
	
float ecode;
float last_ecode;
float single_angle;
float last_singleangle;	
float total_angle;
}MotorData;



void MycanInit(void);
void CanMotorTransmit(uint16_t id,int16_t v1,int16_t v2,int16_t v3,int16_t v4);
MotorData *GetMotorData(void);
//Can_members *Can_Regis(Users_caninit *caninit);
#endif
