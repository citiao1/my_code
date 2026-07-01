#include "conter.h"

//设置参考值，反馈值
RC_Ctl_t *rc_data;
ChassicCmd *Chassic_cmd;
AHRS_FEED *ahrs_feed;
GimbalCmd *gimbal_cmd;
ShootCmd *shoot_cmd;//注意这里定义指针不要与各个文件指针指向的变量名字相同

void GlobalInit(void)
{
	rc_data = GetRCData();//获取遥控器值
	Chassic_cmd = GetChassic();//连接底盘地址
	gimbal_cmd = GetGimbal();//连接云台数据地址
	shoot_cmd = GetShoot();//连接发射装置数据地址
}

void GlobalModeSelect(void)
{

	if(rc_data->rc.s2 == 2)
	{
		Chassic_cmd->Mode = CHASSIS_ROTATE;
	
	}//小陀螺模式
	else if(rc_data->rc.s2 == 3)
	{
		Chassic_cmd->Mode = CHASSIS_NO_FOLLOW;
	}//云台底盘分离
	else if(rc_data->rc.s2 == 1)
	{
		Chassic_cmd->Mode = CHASSIS_FOLLOW_GIMBLE_YAW;
	}//云台底盘跟随
	
	//赋予底盘x,y值
	Chassic_cmd->vx = ((float)(rc_data->rc.ch3 - 1024) * 100.0f/660.0f);//左摇杆上下，控制车前后走
	Chassic_cmd->vy = ((float)(rc_data->rc.ch2 - 1024) * 100.0f/660.0f);//左摇杆左右，控制车左右走
	
	//赋予云台角度值

	gimbal_cmd->Yawdegree += ((rc_data ->rc.ch0 - 1024) * 1.0f/660.0f);
	
	gimbal_cmd->PitchChangedegree = ((rc_data ->rc.ch1 - 1024) * 800.0f/660.0f);
	
	//有待测量······///////////
	if(gimbal_cmd->Pitchdegree >= -70.0f)
	{
		gimbal_cmd->Pitchdegree = -70.0f;
	}
	else if(gimbal_cmd->Pitchdegree <= -120.0f)
	{
		gimbal_cmd->Pitchdegree = -120.0f;
	}
	///////////////
	shoot_cmd->shoot_mood = rc_data->rc.s1;
}
