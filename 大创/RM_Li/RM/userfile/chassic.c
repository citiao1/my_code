#include "chassic.h"
//这是底盘任务，主要是根据云台Yaw轴电机结合机械安装角度偏转，将云台坐标系分解到底盘，实现底盘根据云台朝向为前进
//结合conter文件，那个文件是集中处理接收、连接各个任务的总文件
static ChassicCmd chassic_cmd;
static float vx,vy;
static float motor_speedset[4];
static MotorData *chassic_motordata;
static PidConfig Chassic_pid[4] = 
{

	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},
	{.kp=1.0,.ki= 0.01,.kd=0,.out_max=5000,.i_max=1000,.dead_zone=10},

};
static uint8_t PID_init = 0;
ChassicCmd *GetChassic(void)
{
	return &chassic_cmd;
}

//解决机械角度偏差？？
static void CaculateOstAngle()
{
	float difference = chassic_motordata[4].single_angle - ANGLEOFFSET;
//	//距离机械0位有20度到40度的偏差
	if(difference > 0.0f)
	{
		 chassic_cmd.offset_angle = difference;
	//
	}
	else if(difference <=0 && difference >= -180.0f)
	{
		chassic_cmd.offset_angle = difference;
	}
	else 
	{
		chassic_cmd.offset_angle = difference + 360.0f;
	}
}

static void WheelSpdSet()
{
    //右前轮
    motor_speedset[0] = -(RAD_PS_2_RPM) * (1 / radius) * (-vx + vy + chassic_cmd.vz);
    //左前轮
    motor_speedset[1] = -(RAD_PS_2_RPM) * (1 / radius) * (vx + vy + chassic_cmd.vz);
    //左后轮
    motor_speedset[2] = -(RAD_PS_2_RPM) * (1 / radius) * (vx - vy + chassic_cmd.vz);
    //右后轮
    motor_speedset[3] = -(RAD_PS_2_RPM) * (1 / radius) * (-vx - vy + chassic_cmd.vz);
}
static void WheelPidCltr()
{
	if(PID_init == 0)
	{
		PidAllInit(4,Chassic_pid);
		PID_init = 1;
	}
	PidReturn(&Chassic_pid[0].pid,motor_speedset[0],chassic_motordata[0].speed);
	PidReturn(&Chassic_pid[1].pid,motor_speedset[1],chassic_motordata[1].speed);
	PidReturn(&Chassic_pid[2].pid,motor_speedset[2],chassic_motordata[2].speed);
	PidReturn(&Chassic_pid[3].pid,motor_speedset[3],chassic_motordata[3].speed);

}

//控制底盘对应不同模式云台耦合

void ChassicControl()
{
	chassic_motordata = GetMotorData();//获取所有电机反馈处理信息
	
	switch(chassic_cmd.Mode)
	{
	
		case CHASSIS_NO_FOLLOW :
		{
			chassic_cmd.vz = 0;
		}
			break;
		
		case CHASSIS_FOLLOW_GIMBLE_YAW:
		{
			chassic_cmd.vz = 0;
		}
			break;
		
		case CHASSIS_ROTATE :
		{
			chassic_cmd.vz = 24;
		}
			break;
		
		default:
			break;
	}
	//底盘坐标系耦合置云台，使云台指向始终为前进方向
	CaculateOstAngle();
	
	static float sin_theta,cos_theta;
	cos_theta = (float)cos(chassic_cmd.offset_angle * DEGREE_2_RAD);
	sin_theta = (float)sin(chassic_cmd.offset_angle * DEGREE_2_RAD);
	vx = -(chassic_cmd.vx * cos_theta + chassic_cmd.vy * sin_theta);
	vy = -(-chassic_cmd.vx * sin_theta + chassic_cmd.vy * cos_theta);
	WheelSpdSet();//将分解值组合作用于电机
	WheelPidCltr();//计算每个电机PID
	CanMotorTransmit(0x200,Chassic_pid[0].pid.out,Chassic_pid[1].pid.out,Chassic_pid[2].pid.out,Chassic_pid[3].pid.out);
}
