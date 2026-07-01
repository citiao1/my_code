#include "gimbal.h"
#include "imu.h"
//这是云台任务，主要是Yaw轴根据惯导实现小陀螺（即底盘旋转时，可以根据惯导反馈值以及遥控器设定值使云台一直保持在设定的朝向），Pitch轴则是简单的PID控位置（需测量实际电机安装零位角度）
//结合conter文件，那个文件是集中处理接收、连接各个任务的总文件
static GimbalCmd Gimbal_cmd =
{
	.Pitchdegree = PITCHMIDDLEDEGREE,
};
static float motorYawSet;//设置Yaw值
static float motorPinchSet;//设置Pitch值
static float pitch_change_degree;//记录Pitch改变值
static float total_angle = PITCHMIDDLEDEGREE;//承接Pitch当前值??有待测量
//Pitch控制无需IMU，使用电机的反馈值//遥控器控制角度改变量
//角度应有一个初始值比如水平
static MotorData *Gimbal_motordata;
static AHRS_FEED *Ahrsfeed;
static float fw_value = 0;
uint8_t Yaw_enable_flag = 0;
extern float YawEnableFlag;
static uint8_t PID_init = 0;
static PidConfig Gimbal_pid[4] =
{
	{
	.kp=0,
	.ki=0,
	.kd=0,
	.out_max=135000,
	.i_max=50,
	.dead_zone=10.0,
	.CoefA = 1000.0,
	.CoefB = 2.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit, //| PID_ChangingIntegrationRate,
	.Output_LPF_RC = 0.5,
	},//yaw角度外环
	{
	.kp=53,
	.ki=0.1,
	.kd=0.001,
	.out_max=26000,
	.i_max=1000,
	.dead_zone=10.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit | PID_Derivative_DerivativeFilter,//PID_Derivative_DerivativeFilter | PID_Integral_limit,
	.Output_LPF_RC = 0.1,
	.CoefA = 120.0,
	.CoefB = 20.0,
	},//yaw速度内环
	
	
	{
	.kp=5,
	.ki=0,
	.kd=0,
	.out_max=135000,
	.i_max=50,
	.dead_zone=5.0,
	.CoefA = 1000.0,
	.CoefB = 2.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit, //| PID_ChangingIntegrationRate,
	.Output_LPF_RC = 0.5,
	},//pitch角度外环
	{
	.kp=50,
	.ki=0,
	.kd=0,
	.out_max=26000,
	.i_max=1000,
	.dead_zone=10.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit,// | PID_Derivative_DerivativeFilter,//PID_Derivative_DerivativeFilter | PID_Integral_limit,
	.Output_LPF_RC = 0.1,
	.CoefA = 120.0,
	.CoefB = 20.0,
	},//pitch速度内环
	
};

GimbalCmd *GetGimbal(void)
{
	return &Gimbal_cmd;
}
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
static float GimbalForwardFeedback()
{
	static float KF = 0.06;
	static float speed;
	static float lastspeed;
	static float symbol;
	
	speed = Gimbal_motordata[7].speed;
	symbol = speed / (Abs(speed) + 1);
	
	fw_value = (speed / (Abs(speed) + 1)) * KF * (Abs(speed * 0.50 + lastspeed * 0.50 -symbol * 150));
	lastspeed = speed;
	return fw_value;
}	

static void GimbalPidCltr()
{
	if(PID_init == 0)
	{
		PidAllInit(4,Gimbal_pid);
		PID_init = 1;
	}	

	PidReturn(&Gimbal_pid[0].pid,motorYawSet,Ahrsfeed->YawTotalDegree);//角度外环
	PidReturn(&Gimbal_pid[1].pid,-(Gimbal_pid[0].pid.out + fw_value),Gimbal_motordata[7].speed);//速度内环
	
	PidReturn(&Gimbal_pid[2].pid,motorPinchSet,Gimbal_motordata[8].total_angle);//角度外环
	PidReturn(&Gimbal_pid[3].pid,Gimbal_pid[2].pid.out,Gimbal_motordata[8].speed);//速度内环
}
static float PitchAngleCtl(void)
{
	pitch_change_degree = Gimbal_cmd.PitchChangedegree;
	if(pitch_change_degree >= -10 && pitch_change_degree <= 10)
	{
		total_angle -= 0;	
	}//死区
	else	
	{
		total_angle -= (pitch_change_degree) / 400.0f;
	}
	if(total_angle <= PITCHMAXDEGREE)
	{
		total_angle = PITCHMAXDEGREE;
	}
	if(total_angle >= PITCHMINDEGREE)
	{
		total_angle = PITCHMINDEGREE;
	}
	return total_angle;
}

void GimbalControl()
{
	Gimbal_motordata = GetMotorData();//获取电机数据
	Ahrsfeed = GetAHSRFeed();
	Gimbal_cmd.Pitchdegree = (float)PitchAngleCtl();
	if(YawEnableFlag == 1)
	{
		Gimbal_cmd.Yawdegree = Ahrsfeed->YawTotalDegree;
		YawEnableFlag = 2;
		Yaw_enable_flag = 1;
	}
	if(Yaw_enable_flag == 1)
	{
		motorYawSet = Gimbal_cmd.Yawdegree;//设置目标值
		GimbalForwardFeedback();
		motorPinchSet = Gimbal_cmd.Pitchdegree;
	    GimbalPidCltr();//计算yaw的pid
	}

	CanMotorTransmit(0x2ff,Gimbal_pid[1].pid.out,Gimbal_pid[3].pid.out,0,0);
	
}
