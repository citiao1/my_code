#ifndef MOTOR_H
#define MOTOR_H



#include "bsp_can.h"
#include "pid.h"
#include "DR16.h"
#include "string.h"

#define ECD_ANGLE_COEF  0.0439453152
#define SPEED_SMOOTH_COEF 0.85f
#define CURREANT_SMOOTH_COEF 0.90f
#define RPM_2_ANGLE_PER_SEC 6.0f




// 闭环类型（按位组合）
typedef enum
{
    OPEN_LOOP = 0b0000,
    CURRENT_LOOP = 0b0001,
    SPEED_LOOP = 0b0010,
    ANGLE_LOOP = 0b0100,

    SPEED_AND_CURRENT_LOOP = 0b0011,
    ANGLE_AND_SPEED_LOOP = 0b0110,
    ALL_THREE_LOOP = 0b0111
} Closeloop_Type_e;

// 反馈数据来源（电机自身或外部传感器）
typedef enum
{
    MOTOR_FEED = 0,
    OTHER_FEED
}Feedback_Source_e;


// 正反转标志（目标方向）
typedef enum
{
    MOTOR_DIRECTION_NORMAL = 0,
    MOTOR_DIRECTION_RESERVE = 1
}Motor_Reserve_Flag_e;

// 反馈量正反标志（测量方向）
typedef enum
{
    FEEDBACK_DIRCTION_NORMAL = 0,
    FEEDBACK_DIRCTION_RESERVE = 1
}Feedback_Reserve_Flag_e;

// 电机控制设置：闭环类型、反馈来源、正反标志
typedef struct 
{
    Closeloop_Type_e outer_loop_type;               //最外层闭环
    Closeloop_Type_e close_loop_type;               // 使用几个闭环
    Motor_Reserve_Flag_e motor_reverse_flag;
    Feedback_Reserve_Flag_e feedback_reserve_flag;
    Feedback_Source_e angle_feedback_source;
    Feedback_Source_e speed_feedback_source;
}Motor_Control_Setting_s;

// 电机启停标志
typedef enum 
{
    MOTOR_STOP = 0,
    MOTOR_ENABLE = 1,
}Motor_Working_Type_e;

// 电机控制器：PID 控制器与外部反馈指针集合
typedef struct 
{
    float *other_angle_feedback_ptr;    // 外部角度反馈指针（如IMU）
    float *other_speed_feedback_ptr;    // 外部速度反馈指针

    PIDInstance current_PID;            // 电流环PID
    PIDInstance speed_PID;              // 速度环PID
    PIDInstance angle_PID;              // 角度环PID

    float pid_ref;                      // PID参考值
    float pid_lastref[2];               // 上次参考值（用于记录）
}Motor_Controller_s;

// 电机类型
typedef enum
{
    MOTRO_TYPE_NONE = 0,
    GM6020,
    M3508,
    M2006
}Motor_Type_e;

// 电机控制器参数初始化
typedef struct 
{
    PID_Init_Config_s angle_PID;        // 角度环PID初始化参数
    PID_Init_Config_s speed_PID;        // 速度环PID初始化参数
    PID_Init_Config_s current_PID;      // 电流环PID初始化参数
}Motor_Controller_Init_s;


// 反馈数据（来自电机报文解析）
typedef struct 
{
    uint16_t last_ecd;           
    uint16_t ecd;

    float angle_single_round;
    //角速度(度每秒)
    float rspead;
    float lastrspead;
    int16_t real_current;
    uint8_t temperature;

    float total_angle;
    int32_t total_round;

}motor_measure_s;

// 电机参数初始化
typedef struct 
{
    Motor_Control_Setting_s contorller_setting_init_config;
    Motor_Controller_Init_s contorller_param_init_config;
    Motor_Type_e motor_type;
    CAN_Init_Config_s can_init_config;
}Motor_Init_Config_s;

// 电机实例
typedef struct 
{
    motor_measure_s measure;                    //测量数据
    Motor_Control_Setting_s  motor_settings;    //控制设置
    Motor_Controller_s motor_contorller;           //电机控制器

    CANInstance *motor_can_instance;            //电机can实例

    Motor_Working_Type_e stop_flag;             //启停标志
    //分组发送设置
    uint8_t senter_group;                       
    uint8_t message_num;
    Motor_Type_e motor_type;                    //电机类型

    uint32_t feed_cnt;
    float dt;
}MOTORInstance;

// 初始化电机（注册 CAN，初始化 PID/控制参数，并默认使能）
MOTORInstance *MotorInit(Motor_Init_Config_s *config);
// 使能电机输出
void MotorEnable(MOTORInstance *motor);
// 设置控制参考值（单位依最外层环而定）
void MotorSetRef(MOTORInstance *motor, float ref);
// 设置最外层控制环（ANGLE/SPEED/OPEN）
void MotorOuterLoop(MOTORInstance *motor, Closeloop_Type_e outer_loop);
// 关闭电机输出
void MotorStop(MOTORInstance *motor);
// 统一计算所有电机 PID 并批量发送 CAN 报文
void MotorControl();

#endif 
