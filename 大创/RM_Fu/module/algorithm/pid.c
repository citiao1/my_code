#include "pid.h"

/*
 *****************************************************************************************
 * PID 控制算法模块
 *
 * 特性：
 * - 支持多种改进策略（积分限幅、梯形积分、微分滤波等）
 * - 提供PID初始化与计算接口
 *****************************************************************************************
 */

PIDInstance *ppid[30];          // PID实例注册表（便于调试）
static uint8_t idx = 0;
PIDInstance *wpid[5];
static float am;
static float aref;
static float aout;
static float sm;
static float sref;
static float sout;

/**
 * @brief  梯形积分，实现积分部分的 trapezoid 积分法
 */
void f_Trapezoid_Intergral(PIDInstance *pid)
{
    pid->ITerm = pid->Ki * ((pid->Error * pid->Last_Error)/2)*(pid->dt);
}

/**
 * @brief  积分限幅，避免积分饱和导致震荡
 */
void f_Integral_Limit(PIDInstance *pid)
{
    static float temp_Output,temp_Iout;
    temp_Iout = pid->Iout + pid->ITerm;
    temp_Output = pid->Pout + pid->Iout + pid->Dout;

    if(abs(temp_Output) >= pid->MaxOut)
    {
        if(pid->Error*pid->Iout > 0)
        {
            pid->ITerm = 0;
        }
    }

    if(temp_Iout >= pid->IntegralLimit)
    {
        pid->ITerm = 0;
        pid->Iout = pid->IntegralLimit;
    }

    if(temp_Iout <= -pid->IntegralLimit)
    {
        pid->ITerm = 0;
        pid->Iout = -pid->IntegralLimit;
    }

}



/**
 * @brief  变速积分，根据误差大小调整积分速度
 */
void f_Changing_Integration_Rate(PIDInstance *pid)
{
    if(abs(pid->Error) <= pid->CoefB)
    {
        return;
    }
    if(abs(pid->Error) <= (pid->CoefA + pid->CoefB))
    {
        pid->ITerm *= (pid->CoefA + pid->CoefB - pid->ITerm)/(pid->CoefA/0.8);
    }
    else
    {
        pid->ITerm = 0;
    }
}



/**
 * @brief  微分低通滤波，减少微分噪声
 */
void f_Derivative_LPF_RC(PIDInstance *pid)
{
    pid->Dout = (pid->Dout*pid->dt)/(pid->Derivative_LPF_RC + pid->dt) + 
        (pid->Last_Out*pid->Derivative_LPF_RC)/(pid->Derivative_LPF_RC + pid->dt);        
}



/**
 * @brief  输出低通滤波，提高输出平滑度
 */
void f_Output_LPF_RC(PIDInstance *pid)
{
    pid->Output = (pid->Output*pid->dt)/(pid->Output_LPF_RC + pid->dt) + 
        (pid->Last_Out*pid->Output_LPF_RC)/(pid->Output_LPF_RC + pid->dt);
}



/**
 * @brief  输出限幅，防止输出超出最大值
 */
void f_Output_Limit(PIDInstance *pid)
{
    if(pid->Output >= pid->MaxOut)
    {
        pid->Output = pid->MaxOut;
    }
    if(pid->Output <= -(pid->MaxOut))
    {
        pid->Output = -(pid->MaxOut);
    }
}



/**
 * @brief  PID参数初始化
 */
PIDInstance *PID_Init(PIDInstance *pid ,PID_Init_Config_s *config)
{
    memset(pid, 0, sizeof(PIDInstance));

    pid->Kp = config->Kp;
    pid->Ki = config->Ki;
    pid->Kd = config->Kd;
    pid->KF = config->KF;
    pid->MaxOut = config->MaxOut;
    pid->DeadLimit = config->DeadBand;
    pid->IntegralLimit = config->IntegralLimit;
    pid->Derivative_LPF_RC = config->Derivative_LPF_RC;
    pid->Output_LPF_RC = config->Output_LPF_RC;
    pid->CoefA = config->CoefA;
    pid->CoefB = config->CoefB;
    pid->Improve = config->Improve;



    ppid[idx++] = pid;

    return pid;
}

/**
 * @brief  PID计算函数
 * @param  pid: PID实例
 * @param  measure: 测量值
 * @param  ref: 参考值
 * @retval float: 输出值
 */
float PIDCalculate(PIDInstance *pid, float measure, float ref)
{

    pid->dt = 3;

    pid->Measure = measure;
    pid->Ref = ref;
    pid->Error = pid->Ref - pid->Measure;
    
    if(abs(pid->Error) > pid->DeadLimit)
    {
        pid->Pout = pid->Kp*pid->Error;
        pid->ITerm = pid->Ki*pid->Error*pid->dt;
        pid->Dout = pid->Kd*((pid->Error - pid->Last_Error)/pid->dt);

        if(pid->Improve & PID_Trapezoid_Intergral)
        {
            f_Trapezoid_Intergral(pid);    
        }
        if(pid->Improve & PID_ChangingIntegrationRate)
        {
            f_Changing_Integration_Rate(pid);
        }
        if(pid->Improve & PID_Derivative_DerivativeFilter)
        {
            f_Derivative_LPF_RC(pid);
        }
        if(pid->Improve & PID_Integral_limit)
        {
            f_Integral_Limit(pid);
        }

        pid->Iout += pid->ITerm;
        pid->Output = pid->Pout + pid->Iout + pid->Dout;

        f_Output_LPF_RC(pid);
        f_Output_Limit(pid);

    }
    else
    {
        pid->Output = 0;
        pid->ITerm = 0;
    }

    pid->Last_Dout = pid->Dout;
    pid->Last_Error = pid->Error;
    pid->Last_Iout = pid->Iout;
    pid->Last_ITerm = pid->ITerm;
    pid->Last_Measure = pid->Measure;
    pid->Last_Out = pid->Output;
    pid->Last_Pout = pid->Pout;
   
    wpid[0] = ppid[10];
    wpid[1] = ppid[12];
    am = wpid[0]->Measure;
    aref = wpid[0]->Ref;
    aout = wpid[0]->Output;
    sm = wpid[1]->Measure;
    sref = wpid[1]->Ref;
    sout = wpid[1]->Output; 

    return pid->Output;
    // return 0;


}

/**
 * @brief  获取PID实例列表
 */
PIDInstance **Getpid(void)
{
    return ppid;
}




















