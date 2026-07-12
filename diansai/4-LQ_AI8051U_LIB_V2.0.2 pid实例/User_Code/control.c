#include "control.h"

int16 ax, ay, az;  //LSM6读加速度
int16 gx,gy, gz;  //LSM6读角速度
double Gyroz;  //偏航角速度
double Angle_gyro_z;  //积分角度
int L,L2,M,R2,R;
double left_1,left_2,AM,right_1,right_2;  //归一化后的电感值
double eleValue, eleSub, eleAdd;	// 差比和
double elemid = 0;					// 转向环目标位置偏差
double eleout = 0;					// 转向环输出值
double encoder_L,encoder_R;  //左右轮编码器数据
double LEFT_MOTOR_Duty, RIGHT_MOTOR_Duty;				// 左右电机占空比
PID Left_MOTOR_PID, Right_MOTOR_PID, Turn_PID_ele;  // 三个结构体变量

unsigned char run = 0;  // 电机启停标志位

double Turn_ele[2] = {0.65,1.88};
double Left_MOTOR[3] = {9,3.5, 0};   // 左轮速度环 PID 参数
double Right_MOTOR[3] = {9, 3.5, 0};  // 右轮速度环 PID 参数
double Left_High_Speed, Right_High_Speed, High_Speed;	// 左右轮目标速度、基础目标速度
double battery_voltage;
int ben_flg = 0;
double Angle;
/**
 * @brief 函数：获取电感值
 */
void Get_Inductance()
{
	L = Get_ADCResult(5);  //左横
	L2 = Get_ADCResult(1); //左竖
	M = Get_ADCResult(2);  //中间
	R2 = Get_ADCResult(4);  //右竖
	R = Get_ADCResult(3);  //右横
}

/**
 * @brief 方向环控制函数，算出最终左右轮目标速度
 */
void Dir_Control()
{
	
//	if(L < 15 && R < 15 && L2 < 15 && R2 < 15) run = 0;
//	else run = 1;
//	
//	if(run == 1)
//	{
//		if(eleout > 60)
//    High_Speed = 30;
//		else if(eleout > 30)
//    High_Speed = 40;
//		else if(eleout > 10)
//    High_Speed = 50;
//		else
//    High_Speed = 60;  
//		High_Speed = 30;
//	}
//	else
//	{
//		High_Speed = 0;
//		Left_High_Speed = 0;
//		Right_High_Speed = 0;
//	}
	
	Get_Inductance();
	
	LSM6DSR_Read_Data(&ax,&ay,&az,&gx,&gy,&gz);
//	lsm6dsr_read_raw_data(&ax,&ay,&az,&gx,&gy,&gz);
//	
	eleSub = (double)(L - R + L2 - R2);
	eleAdd = (double)(L + R + L2 + R2 + M + 10);
	eleValue = range_protect(eleSub/eleAdd * 100.0f,-100.0,100.0);
	
//	Angle_calculate();
//	
//	if(ben_flg == 0)
//	{
		 eleout = PlacePID_Control(&Turn_PID_ele,eleValue,elemid,Turn_ele);
//	}
//	
		eleout = range_protect(eleout,-85,85);
	
	
//		if(eleout <= 0)  // 需要左转
//		{
//			Angle = (-eleout);
//			Left_High_Speed = High_Speed  - Angle*0.65 +gz*0.001;  //0.001
//			Right_High_Speed = High_Speed + Angle*0.55*0.7 -gz*0.001;  // 外轮加得少一点
//		}
//		else  // 需要右转
//		{
//			Angle = (eleout) ;
//			Left_High_Speed = High_Speed  + Angle*0.55*0.7-gz*0.001;  // 外轮加得少一点
//			Right_High_Speed = High_Speed  - Angle*0.65+gz*0.0015;
//		}
		High_Speed = 30;  //test
		Calculate_Differential_Drive();
	
}

void Motor_Control()
{
	encoder_L = Read_Encoder(1);  
	encoder_R = -1 * Read_Encoder(2); 

	
	// 下面有 A、B 两种占空比赋值方式，A 用于测试电机左右和正反转是否对应，B 可实现速度闭环
	
	// A：人为给定占空比，用于测试：①左右轮不要搞反  ②正反转不要搞反
//	LEFT_MOTOR_Duty = 2000;  	// 2000 效果：左轮正转，速度较快
//	RIGHT_MOTOR_Duty = -1200;	//  -1200 效果：右轮反转，速度较慢
	
	// B：闭环计算占空比，在确认编码器和电机对应无误后，可实现闭环
	LEFT_MOTOR_Duty += PID_Realize(&Left_MOTOR_PID, encoder_L, Left_High_Speed, Left_MOTOR);
	RIGHT_MOTOR_Duty += PID_Realize(&Right_MOTOR_PID, encoder_R, Right_High_Speed, Right_MOTOR);
	
	if(LEFT_MOTOR_Duty>0)
	{
			LEFT_MOTOR_Duty = range_protect(LEFT_MOTOR_Duty, 0, MOTOR_MAX);
	}
	else
	{
			LEFT_MOTOR_Duty = range_protect(LEFT_MOTOR_Duty, -MOTOR_MAX, 0);
	}

	if(RIGHT_MOTOR_Duty>0)
	{
			RIGHT_MOTOR_Duty = range_protect(RIGHT_MOTOR_Duty, 0, MOTOR_MAX);
	}
	else
	{
			RIGHT_MOTOR_Duty = range_protect(RIGHT_MOTOR_Duty, -MOTOR_MAX, 0);
	}
	Motor_Ctrl(-1*(int16)RIGHT_MOTOR_Duty, (int16)LEFT_MOTOR_Duty);  //正负号需要自己确定
}

/**
 * @brief 综合控制函数，在中断中调用
 */

void Ultima_Control()
{

	Dir_Control();  // 转向控制
//	Motor_Control();  //电机控制

}

double range_protect(double duty, double min, double max)
{
    return duty >= max ? max : (duty <= min ? min : duty);
}

/**
 * @brief 位置式 PID（用于转向控制）
 */
double PlacePID_Control(PID*sptr, double NowPiont, double SetPoint, double *Turn_PID)
{
	double Output;  // 本次输出
	
	sptr->KP = *Turn_PID;  // 参数赋值
	sptr->KD = *(Turn_PID+1);
	
	sptr->iError = SetPoint - NowPiont;  // 当前误差 = 目标值 - 实际值
	
	Output = sptr->KP * sptr->iError  // 比例项
		   + sptr->KD * (sptr->iError - sptr->LastError);  // 微分项
	
	sptr->LastError = sptr->iError;  // 更新误差
	
	return Output;
}

/**
 * @brief 增量式 PID（用于左右轮电机闭环控制）
 */
double PID_Realize(PID* sptr, double ActualSpeed, double SetSpeed, double *MOTOR_PID)
{
    double Increase;        // 单次PID输出
//    double iTermLimited;    // 限幅后的积分项

    // 参数赋值
    sptr->KP = *MOTOR_PID;
    sptr->KI = *(MOTOR_PID + 1);
    sptr->KD = *(MOTOR_PID + 2);

    // 计算当前误差
    sptr->iError = SetSpeed - ActualSpeed;

    // 积分项限幅：限制 KI * iError 在 [-900, +900]
//    iTermLimited = sptr->KI * sptr->iError;
//    if (iTermLimited > 1000) iTermLimited = 1000;
//    else if (iTermLimited < -1000) iTermLimited = -1000;

    // PID增量计算
    Increase = sptr->KP * (sptr->iError - sptr->LastError)
             + sptr->KI * sptr->iError;
             + sptr->KD * (sptr->iError - 2 * sptr->LastError + sptr->PrevError);

    // 更新误差
    sptr->PrevError = sptr->LastError;
    sptr->LastError = sptr->iError;

    return Increase;
}


void Angle_calculate()  //gz 是int16 有符号整形，范围是-32768 - 32768（开区间） 因此此处是错的
{     
	  Gyroz=(double)gz;
    if(Gyroz<32768)Gyroz = - (Gyroz / 16.4);
    if(Gyroz>=32768)Gyroz = + (65535 - Gyroz) / 16.4;
 
    Angle_gyro_z += Gyroz * 0.002;  // 根据中断周期进行微调
}
/**
* @brief 归一化函数，将输入值映射到 1 ~ 100 范围
* @param value 输入初始电感值（此时为整型）
* @return double 归一化后的值（1 ~ 100）
*/
double Adc_Normalize(int value, double min, double max)
{
		double normalized = 0;
		// 计算归一化值
		normalized = (double)(value - min) / (max - min) * 100.0f;
		// 限幅保护，确保返回值在 1 ~ 100 范围内
		normalized = range_protect(normalized, 1.0, 100.0);
		return normalized;
}
void Calculate_Differential_Drive() // 差速计算
{
	float k = 0; // 差速比例系数
	k = eleout * 0.01; // 将 -100 ~ 100 缩放成 -1 ~ 1
	k = range_protect(k, -0.65, 0.65); // 限制到 -0.65 ~ 0.65，实现差速限幅
	// 计算左右轮目标速度
	if(k < 0) // 左转
	{
		k *= -1;
		Left_High_Speed = High_Speed * (1 - k);
		Right_High_Speed= High_Speed * (1 + k*0.2); // 加少减多
	}
	else // 右转
	{
		Left_High_Speed = High_Speed * (1 + k*0.2); // 加少减多
		Right_High_Speed= High_Speed * (1 - k);
	}
}
void ADC_Display()
{
	/*以下显示归一化后电感值*/
				char txt[20];
//				sprintf(txt, "ADV:%05d ", Get_ADCResult(ADC_CH10_P02)); // ADC_CH8-P00做电源电压 11.0V->1700左右，12.6V->1970左右
//				LCD_ShowStr_16(0,1,txt,u16_RED_IPS,u16_BLACK);         // 显示电源电压值
//				sprintf(txt, "AL1:%d L1:%05d",(int)left_1,L); // ADC5-P15
//        LCD_ShowStr_16(0,2,txt,u16_RED_IPS,u16_BLACK); 
//				sprintf(txt, "AL2:%d L2:%05d",(int)left_2,L2); // ADC1P11
//        LCD_ShowStr_16(0,3,txt,u16_RED_IPS,u16_BLACK);      
//				sprintf(txt, "AM:%d M:%05d",(int)AM,M); // ADC2-P12
//        LCD_ShowStr_16(0,4,txt,u16_RED_IPS,u16_BLACK);                     // 显示通道5的ADC值
//				sprintf(txt, "AR2:%d R2:%05d",(int)right_2,R2); // ADC4-P14
//        LCD_ShowStr_16(0,5,txt,u16_RED_IPS,u16_BLACK);                    // 显示通道7的ADC值
//				sprintf(txt, "AR1:%d R1:%05d",(int)right_1,R); // ADC3-P13
//        LCD_ShowStr_16(0,6,txt,u16_RED_IPS,u16_BLACK);                              // 显示通道7的ADC值
	
				sprintf(txt, "L1:%05d ", L); // ADC5-P15
        LCD_ShowStr_16(0,2,txt,u16_RED_IPS,u16_BLACK); 
        sprintf(txt, "L2:%05d ", L2); // ADC1P11
        LCD_ShowStr_16(0,3,txt,u16_RED_IPS,u16_BLACK);      
        sprintf(txt, "M:%05d ", M); // ADC2-P12
        LCD_ShowStr_16(0,4,txt,u16_RED_IPS,u16_BLACK);                     // 显示通道5的ADC值
        sprintf(txt, "R2:%05d ", R2); // ADC3-P13
        LCD_ShowStr_16(0,5,txt,u16_RED_IPS,u16_BLACK);                    // 显示通道7的ADC值
        sprintf(txt, "R1:%05d ", R); // ADC4-P14
        LCD_ShowStr_16(0,6,txt,u16_RED_IPS,u16_BLACK);
				sprintf(txt,"eV:%03d eo:%03d",(int)eleValue,(int)eleout);
				LCD_ShowStr_16(0,7,txt,u16_RED_IPS,u16_BLACK);
				sprintf(txt,"left_v:%d right_v:%d",(int)Left_High_Speed,(int)Right_High_Speed);
//				sprintf(txt,"gz:%05d",gz);
				LCD_ShowStr_16(0,1,txt,u16_RED_IPS,u16_BLACK);
	
}