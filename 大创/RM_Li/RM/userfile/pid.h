#include "main.h"
#ifndef _pid__H__
#define _pid__H__

typedef enum 
{
  PID_IMPROVE_NONE = 0x00,
  PID_Integral_limit = 0x01,
  PID_Trapezoid_Intergral = 0x02,
  PID_ChangingIntegrationRate = 0x04 ,
  PID_Derivative_DerivativeFilter = 0x08,
}PID_Improvement_e;
typedef struct 
{
float kp;
float ki;
float kd;
float dt;
float out_max;
float i_max;
float Derivative_LPF_RC;
float Output_LPF_RC;
	
float target;
float feedback;
float error[2];
	
float p_out;
float i_out;
float d_out;

float ITerm;
float CoefA;
float CoefB;	

float out;
float Last_Out;	
float dead_zone;
	
PID_Improvement_e improve;
}PidStructure;

typedef struct
{
PidStructure pid;
float kp;
float ki;
float kd;
	
float Derivative_LPF_RC; 
float Output_LPF_RC;
float CoefA;
float CoefB;
	
float out_max;
float i_max;
float dead_zone;
PID_Improvement_e improve;
}PidConfig;
void PidAllInit(uint8_t number,PidConfig * pidinitstruct);
void PidInit(PidStructure*pid,PidConfig * pidinitstruct);
//void PidInit(PidStructure*pid,float kp,float ki,float kd,float out_max,float i_max,float dead_zone);
float PidReturn(PidStructure*pid,float target,float feedback);

#endif
