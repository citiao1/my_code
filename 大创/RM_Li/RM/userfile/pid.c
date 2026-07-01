#include "stm32f4xx.h"                  // Device header
#include "pid.h"
#include <math.h>
#include <stdio.h>

static float Abs(float value)
{
	if(value > 0.0f)
	{
		value = value;
	}
	else if(value < 0.0f)
	{
		value = -value;
	}
	return value;
}
void PidInit(PidStructure*pid,PidConfig * pidinitstruct)
{
pid->kp=pidinitstruct->kp;
pid->ki=pidinitstruct->ki;
pid->kd=pidinitstruct->kd;
pid->out_max=pidinitstruct->out_max;	
pid->i_max=pidinitstruct->i_max;
pid->Derivative_LPF_RC = pidinitstruct->Derivative_LPF_RC;
pid->Output_LPF_RC = pidinitstruct->Output_LPF_RC; 
pid->CoefA = pidinitstruct->CoefA;
pid->CoefB = pidinitstruct->CoefB;
pid->improve = pidinitstruct->improve;
pid->error[0] = 0;
pid->error[1] = 0;
pid->i_out = 0;	
pid->dead_zone = pidinitstruct->dead_zone;
}
//积分限幅
void f_Integral_Limit(PidStructure *pid)
{
    static float temp_Output,temp_Iout;
    temp_Iout = pid->i_out + pid->ITerm;
    temp_Output = pid->p_out + pid->i_out + pid->d_out;

    if(Abs(temp_Output) >= pid->out_max)
    {
        if(pid->error[0] * pid->i_out > 0)
        {
            pid->ITerm = 0;
        }
    }
    if(temp_Iout >= pid->i_max)
    {
        pid->ITerm = 0;
        pid->i_out = pid->i_max;
    }
    if(temp_Iout <= -pid->i_max)
    {
        pid->ITerm = 0;
        pid->i_out = -pid->i_max;
    }
}
//微分滤波
void f_Derivative_LPF_RC(PidStructure*pid)
{
	pid->d_out = (pid->d_out * pid->dt)/(pid->Derivative_LPF_RC + pid->dt) + 
        (pid->Last_Out * pid->Derivative_LPF_RC)/(pid->Derivative_LPF_RC + pid->dt); 

}
//变速积分
void f_Changing_Integration_Rate(PidStructure *pid)
{
    if(Abs(pid->error[0]) <= pid->CoefB)
    {
        return;
    }
    if(Abs(pid->error[0]) <= (pid->CoefA + pid->CoefB))
    {
        pid->ITerm *= (pid->CoefA + pid->CoefB - pid->ITerm)/(pid->CoefA/0.8);
    }
    else
    {
        pid->ITerm = 0;
    }
}
//输出滤波
void f_Output_LPF_RC(PidStructure *pid)
{
    pid->out = (pid->out * pid->dt)/(pid->Output_LPF_RC + pid->dt) + 
        (pid->Last_Out*pid->Output_LPF_RC)/(pid->Output_LPF_RC + pid->dt);
}

//输出限幅
void f_Output_Limit(PidStructure *pid)
{
    if(pid->out >= pid->out_max)
    {
        pid->out = pid->out_max;
    }
    if(pid->out <= -(pid->out_max))
    {
        pid->out = -(pid->out_max);
    }
}
void PidAllInit(uint8_t number,PidConfig * pidinitstruct)
{
	
	for(uint8_t i=0;i<number;i++)
	{
	PidInit(&pidinitstruct[i].pid,&pidinitstruct[i]);
	
	}
}
float PidReturn(PidStructure*pid,float target,float feedback)
{
	pid->dt = 3;
	pid->target=target;
	pid->feedback=feedback;	
	pid->error[1]=pid->error[0];
	pid->error[0]=pid->target - pid->feedback;
	// 应用死区：如果误差绝对值小于阈值，则设为 0
	if(Abs(pid->error[0]) > pid->dead_zone)
	{
		pid->p_out = pid->kp*pid->error[0];
		pid->ITerm = pid->ki*pid->error[0];
		pid->d_out=pid->kd*(pid->error[0]-pid->error[1]);

		if(pid->improve & PID_ChangingIntegrationRate)
		{
			f_Changing_Integration_Rate(pid);
		}
		
		if(pid->improve & PID_Derivative_DerivativeFilter)
		{
			f_Derivative_LPF_RC(pid);
		}
		if(pid->improve & PID_Integral_limit)
		{
			f_Integral_Limit(pid);
		}
		pid->i_out += pid->ITerm;
		pid->out=pid->p_out+pid->i_out+pid->d_out;
		f_Output_LPF_RC(pid);
		f_Output_Limit(pid);
	}
	else
	{
		pid->ITerm = 0;
		pid->out = 0;
	}
	pid->Last_Out = pid->out;
	return pid->out;
}


