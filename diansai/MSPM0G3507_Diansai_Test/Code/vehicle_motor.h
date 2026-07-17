#ifndef VEHICLE_MOTOR_H
#define VEHICLE_MOTOR_H

#include <stdint.h>

/*
 * 电机模块对上层只暴露左右轮百分比输出。
 * 数值范围为 -100..100，正负号表示方向，0 表示两输入拉低并滑行停车。
 * AT8236 的 20 kHz 慢衰减 PWM、定时器通道和换向保护均封装在 .c 中。
 */
typedef struct
{
    int8_t left_percent;
    int8_t right_percent;
} VehicleMotorState;

/* 初始化两路 H 桥并立即输出 0；上电后只调用一次。 */
void VehicleMotor_Init(void);

/* 设置左右轮百分比，超出 -100..100 的输入会在模块内部限幅。 */
void VehicleMotor_Set(int32_t left_percent, int32_t right_percent);

/* 关闭四个桥臂 PWM，并同步清零状态快照。 */
void VehicleMotor_Stop(void);

/* 返回模块内部只读快照；调用者不得修改或长期缓存其中内容。 */
const VehicleMotorState *VehicleMotor_GetState(void);

#endif
