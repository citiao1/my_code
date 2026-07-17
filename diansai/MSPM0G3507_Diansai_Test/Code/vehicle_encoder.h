#ifndef VEHICLE_ENCODER_H
#define VEHICLE_ENCODER_H

#include <stdint.h>

/*
 * 编码器快照中的累计距离单位为原始计数，速度单位统一为 mm/s。
 * filtered_*_mm_s_float 保留给控制器，整数版本用于串口协议和界面显示。
 * 左右轮均使用本车实测的 1867 count/m 标定值。
 */
typedef struct
{
    int32_t total_left;
    int32_t total_right;
    int32_t raw_left_mm_s;
    int32_t raw_right_mm_s;
    int32_t filtered_left_mm_s;
    int32_t filtered_right_mm_s;
    float filtered_left_mm_s_float;
    float filtered_right_mm_s_float;
} VehicleEncoderState;

/* 配置两路方向编码器并清零硬件/软件计数。 */
void VehicleEncoder_Init(void);

/* 在临界区内同时清零中断计数、累计里程和速度滤波器。 */
void VehicleEncoder_Reset(void);

/*
 * 读取本周期增量并更新速度。elapsed_ms 必须是实际经过的毫秒数，模块会按
 * 0.65*旧值 + 0.35*新值进行低通滤波，避免任务延迟造成虚假速度尖峰。
 */
void VehicleEncoder_Update(uint32_t elapsed_ms);

/* 返回模块内部只读快照；下次 Update/Reset 后内容会原地更新。 */
const VehicleEncoderState *VehicleEncoder_GetState(void);

#endif
