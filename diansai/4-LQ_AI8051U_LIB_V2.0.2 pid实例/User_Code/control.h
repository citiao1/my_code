#ifndef __CONTROL_H_  //避免重复定义
#define __CONTROL_H_

#include "include.h"

typedef struct PID
{
	double iError;  // 本次偏差
    double LastError;  // 上次偏差
    double PrevError;  // 上上次偏差
    double KP;
    double KI;
    double KD;
}PID;

#define MOTOR_MAX 3200  // 注意满占空比为3200（40000000/12500）

void Get_Inductance(void);
void Angle_calculate(void);
double PID_Realize(PID*sptr, double ActualSpeed, double SetSpeed, double *MOTOR_PID);   // 增量式 PID
double PlacePID_Control(PID*sptr, double NowPiont, double SetPoint, double *Turn_PID);  //位置式PID
double Adc_Normalize(int value, double min, double max);
double range_protect(double duty, double min, double max);  //限幅函数
void Dir_Control();
void Motor_Control();
void Ultima_Control();
void Calculate_Differential_Drive(); // 差速计算
extern int16 ax, ay, az;  //LSM6读加速度
extern int16 gx,gy, gz;  //LSM6读角速度
extern int L,L2,M,R2,R;
extern double left_1,left_2,AM,right_1,right_2;  //归一化后的电感值
extern double eleValue, eleSub, eleAdd,eleout,elemid;	// 差比和
extern double Gyroz;  //偏航角速度
extern double Angle_gyro_z;  //积分角度
extern double encoder_L,encoder_R;  //左右轮编码器数据
extern double LEFT_MOTOR_Duty, RIGHT_MOTOR_Duty;				// 左右电机占空比
extern PID Left_MOTOR_PID, Right_MOTOR_PID, Turn_PID_ele;  // 三个结构体变量
extern double Left_MOTOR[3];   // 左轮速度环 PID 参数
extern double Right_MOTOR[3];  // 右轮速度环 PID 参数
extern double Left_High_Speed, Right_High_Speed, High_Speed;	// 左右轮目标速度、基础目标速度
extern int ben_flg;
extern double battery_voltage;
extern double Turn_ele[2];
extern unsigned char run;
extern double Angle;
#endif