#ifndef VEHICLE_IMU_H
#define VEHICLE_IMU_H

#include <stdint.h>

/*
 * LSM6DSR 对外数据快照。
 * yaw_rate_dps 和 yaw_deg 使用车辆坐标系：左转为正、右转为负。
 * pitch10/roll10/yaw10 是 0.1 度整数形式，供低带宽串口协议直接发送。
 * id/ok 和 mosi/miso 引脚号可用于判断自动探测到的 SPI 接线方向。
 */
typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t pitch10;
    int16_t roll10;
    int16_t yaw10;
    float yaw_deg;
    float yaw_rate_dps;
    uint8_t id;
    uint8_t ok;
    uint8_t mosi_pin_number;
    uint8_t miso_pin_number;
} VehicleImuState;

/* 自动尝试 PA13/PA14 两种数据方向并初始化传感器；成功返回 1。 */
uint8_t VehicleImu_Init(void);

/*
 * 阻塞采集约 2.9 秒的静止样本，计算三轴陀螺仪零偏并清零相对航向。
 * 传感器已通过初始化检查并完成标定时返回 1；IMU 不可用时返回 0。
 */
uint8_t VehicleImu_CalibrateGyro(void);

/*
 * 采集一次六轴数据并更新姿态。vehicle_stationary 是上层依据电机和编码器
 * 给出的静止许可，只有该值为 1 且传感器数据也满足静止条件时才跟踪温漂。
 */
void VehicleImu_Update(uint32_t now_ms, uint8_t vehicle_stationary);

/* 只清零积分得到的相对航向，不重新采集陀螺仪零偏。 */
void VehicleImu_ResetYaw(void);

/* 返回模块内部只读快照；下次采样后内容会原地更新。 */
const VehicleImuState *VehicleImu_GetState(void);

#endif
