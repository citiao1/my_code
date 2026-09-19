#include "control.h"	
float Velocity_KP = 100,Velocity_KI = 10;			//增量式PI参数，用于电机速度控制
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/
int Motor_Pwm=0;
u16 Test_cnt;
int Target_Encoder=8,Current_Encoder;
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET ) 
	{
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
		Current_Encoder=Read_Encoder(2);//读取编码器数值
		Test_cnt++;
		if(Test_cnt==500)//每5秒转换一次转动方向
		{
			Test_cnt = 0;
			Target_Encoder = -Target_Encoder;//因为实现了速度闭环模式，所以改变目标值的符号也就实现了改变电机的转向。
		}
		EN1=1;//使能无刷电机,高电平使能，低电平失能
		Motor_Pwm = Incremental_PI(Current_Encoder,Target_Encoder);//增量式PI，转换成驱动电机的PWM
		Motor_Pwm = PWM_Limit(Motor_Pwm,7199,-7199);
		Set_Pwm(Motor_Pwm);//驱动电机
	}		
}

/**************************************************************************
函数功能：赋值给PWM寄存器
入口参数：PWM
返回  值：无
*************************************************************************/
void Set_Pwm(int motor_a)
{
        
		if(motor_a<0)			DIR1=1;//正转
        else			        DIR1=0;//反转
		PWMA=myabs(motor_a);	
}


/**************************************************************************
函数功能：绝对值函数
入口参数：long int
返回  值：unsigned int
**************************************************************************/
u32 myabs(long int a)
{ 		   
	  u32 temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}
/**************************************************************************
Function: PWM_Limit
Input   : IN;max;min
Output  : OUT
函数功能：限制PWM赋值
入口参数: IN：输入参数  max：限幅最大值  min：限幅最小值 
返回  值：限幅后的值
**************************************************************************/	 	
float PWM_Limit(float IN,int max,int min)
{
	float OUT = IN;
	if(OUT>max) OUT = max;
	if(OUT<min) OUT = min;
	return OUT;
}
/**************************************************************************
函数功能：增量PI控制器
入口参数：编码器测量值，目标速度
返回  值：电机PWM
根据增量式离散PID公式 
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)代表本次偏差 
e(k-1)代表上一次的偏差  以此类推 
pwm代表增量输出
在我们的速度控制闭环系统里面，只使用PI控制
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)
**************************************************************************/

int Incremental_PI(int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Target-Encoder;                					//计算偏差
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   	//增量式PI控制器
	 Last_bias=Bias;	                   					//保存上一次偏差 
	 return Pwm;                         					//增量输出
}
