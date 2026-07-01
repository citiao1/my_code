#ifndef CHASSIC_H
#define CHASSIC_H

#include "main.h"
#include "motor.h"

// 底盘参数定义
#define pi                  3.1415926536f           // 圆周率
#define CHASSICLENGH        0.37f                   // 底盘长度（米）
#define CHASSICWHEIGH       0.37f                   // 底盘宽度（米）
#define DEGREE_2_RAD        0.0174529252f           // 度转弧度系数
#define RAD_2_DEGREE        57.216847881f           // 弧度转度系数
#define radius              0.150f                  // 麦克纳姆轮半径（米）
#define RAD_PS_2_RPM        (30.0f/pi)              // 弧度/秒转RPM系数

// 底盘控制模式枚举
typedef enum
{
    CHASSIS_ZERO_FORCE,             // 零力模式：底盘停止
    CHASSIS_NO_FOLLOW,              // 不跟随模式：底盘不跟随云台
    CHASSIS_FOLLOW_GIMBLE_YAW,      // 跟随云台模式：底盘跟随云台偏航角
    CHASSIS_ROTATE                  // 旋转模式：小陀螺模式
} Chassis_Mode_e;

// 底盘控制命令结构体
typedef struct
{
    float vx;                       // X方向速度（m/s）
    float vy;                       // Y方向速度（m/s）
    float wz;                       // 角速度（rad/s）
    float offset_angle;             // 偏移角度（度，用于云台跟随模式）
    int chassic_speed_buff;         // 底盘速度缓冲（未使用）
    Chassis_Mode_e Chassis_Mode;    // 底盘控制模式
} Chassic_Ctrl_Cmd;

// 底盘电机配置结构体
typedef struct
{
    CAN_Init_Config_s can_init_config;
    Motor_Control_Setting_s contorller_setting_init_config;
    Motor_Reserve_Flag_e motor_reverse_flag;
} Chassic_Motor_Config;


// 函数声明
void Chassistask(void);             // 底盘控制任务（每个控制周期调用）
void ChassisInit(void);             // 底盘初始化函数
Chassic_Ctrl_Cmd *GetChassisCmd(void);  // 获取底盘命令结构体指针

#endif

