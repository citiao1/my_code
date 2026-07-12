#ifndef __CHASSIS_H_
#define __CHASSIS_H_

#include "pid.h"

// 底盘控制结构体
typedef struct
{
    int16 target_speed;        // 目标基础速度
    int current_deviation;    // 当前赛道偏离程度（外环输出）
    int16 left_speed;         // 左轮目标速度
    int16 right_speed;        // 右轮目标速度
} Chassis_TypeDef;

// 环岛状态机枚举
typedef enum {
    LOOP_NORMAL = 0,   // 正常巡线
    LOOP_APPROACH,     // 逼近环岛
    LOOP_TURN_IN,      // 强制入环
    LOOP_IN_LOOP,      // 环内巡线
    LOOP_EXITING       // 强制出环
} RoundaboutState_e;

typedef enum {
    LOOP_NONE = 0,
    LOOP_LEFT,
    LOOP_RIGHT
} LoopType_e;


// 🌟 新增：统一的 IMU 数据缓存结构体
typedef struct {
    int16 ax, ay, az;
    int16 gx, gy, gz;
} IMU_Data_t;

// 函数声明
void Chassis_Init(void);
void Chassis_Control(void);
int Calculate_Deviation(void);  // 差比和算法计算偏离值
void Battery_Protection_Task(void);
void Roundabout_Detect_Task(void);

extern Chassis_TypeDef chassis;
extern int16 direction_output;
extern int16 actual_yaw_rate;            // 内环输入：真实的偏航角速度
extern int current_adc;
extern int16 target_yaw_rate;
extern double car_angle;
extern int set_angle_calculation;
extern RoundaboutState_e loop_state;
extern LoopType_e loop_type;
extern IMU_Data_t imu_data;
#endif