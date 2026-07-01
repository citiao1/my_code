#include "shoot.h"
//这是射击任务，比较简单
//结合conter文件，那个文件是集中处理接收、连接各个任务的总文件
static ShootCmd Shoot_cmd;
static float motorBulletDialSet;//拨弹盘目标值
static float motorFrictionSet1;//摩擦轮左目标值
static float motorFrictionSet2;//摩擦轮右目标值？？有待考究
static MotorData *Shoot_motordata;
static PidConfig Shoot_pid[3] = 
{
	{
	.kp=1,
	.ki=0,
	.kd=0,
	.out_max=50000,
	.i_max=50,
	.dead_zone=10.0,
	.CoefA = 1000.0,
	.CoefB = 2.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit, //| PID_ChangingIntegrationRate,
	.Output_LPF_RC = 0.1,
	},//拨弹盘单环速度PID？？加电流环？？
	
	
	{
	.kp=1,
	.ki=0,
	.kd=0,
	.out_max=50000,
	.i_max=50,
	.dead_zone=10.0,
	.CoefA = 1000.0,
	.CoefB = 2.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit, 
	.Output_LPF_RC = 0.1,
	},//左单环速度PID？？加电流环？？
	
	
	{
	.kp=1,
	.ki=0,
	.kd=0,
	.out_max=50000,
	.i_max=50,
	.dead_zone=10.0,
	.CoefA = 1000.0,
	.CoefB = 2.0,
	.Derivative_LPF_RC = 2.0,
	.improve = PID_Integral_limit, 
	.Output_LPF_RC = 0.1,
	},//右单环速度PID？？加电流环？
};
static uint8_t PID_init = 0;

ShootCmd *GetShoot(void)
{
 return &Shoot_cmd;
};
static void GimbalPidCltr()
{

	if(PID_init == 0)
	{
		PidAllInit(3,Shoot_pid);
		PID_init = 1;
	}
	//计算PID
	PidReturn(&Shoot_pid[0].pid,motorBulletDialSet,Shoot_motordata[4].speed);
	PidReturn(&Shoot_pid[1].pid,motorFrictionSet1,Shoot_motordata[5].speed);
	PidReturn(&Shoot_pid[2].pid,motorFrictionSet2,Shoot_motordata[6].speed);
	///////可能要改
}
void ShootControl()
{
	Shoot_motordata = GetMotorData();//获取电机数据
	switch(Shoot_cmd.shoot_mood)
	{
		case MOVENONE:
		{
			motorBulletDialSet = 0;
			motorFrictionSet1 = 0;
			motorFrictionSet2 = 0;
		}
			break;//s1 = 2都不动
		
		case MOVEHEAD:
		{
			motorBulletDialSet = 0;
			motorFrictionSet1 = FRICTIONLEFTSPEED;
			motorFrictionSet2 = FRICTIONRIGHTSPEED;
		}
			break;//s1 = 3摩擦轮动
		
		case MOVEALL:
		{
			motorBulletDialSet = BULLETSPEED;
			motorFrictionSet1 = FRICTIONLEFTSPEED;
			motorFrictionSet2 = FRICTIONRIGHTSPEED;
		}
			break;//s1 = 1全动
		
		case MOVEMOUSE:
		{
			motorBulletDialSet += 65;
			motorFrictionSet1 = 50000;
			motorFrictionSet2 = 50000;
			if(motorBulletDialSet >= 10500)
			{
				motorBulletDialSet = 10500;
			}
		}
			break;//键鼠相关待定
		
		case MOVERESERVE:
		{
			motorBulletDialSet = -2300;
		}
			break;//键鼠相关待定
		
		default:
			break;
	}
	GimbalPidCltr();
	CanMotorTransmit(0x1ff,Shoot_pid[0].pid.out,Shoot_pid[1].pid.out,Shoot_pid[2].pid.out,0);
}
