#ifndef __MID_IMU_H__
#define __MID_IMU_H__

/*******************************************************************************
 * @file    mid_imu.h
 * @brief   IMU / 陀螺仪中间层（Middle Layer）
 *
 * @note    本模块是本工程接触陀螺仪硬件的唯一入口，封装了：
 *          - LSM6DSV16X 初始化
 *          - 原始角速度数据读取（LSB -> deg/s 转换）
 *          - 陀螺仪 Z 轴零偏自动校准
 *          - SFLP 6 轴融合初始化 + 偏航角 (yaw) 实时读取
 *
 *          隔离原则：
 *          - 业务层（chassis / scheduler）只通过本模块使用陀螺仪，
 *            严禁直接 include LQ_lsm6dsv16x.h
 *          - 零偏通过全局参数 g_param.gyro_offset 暴露给业务层
 *          - 偏航角 (yaw_deg) 通过 MID_IMU_ReadYawDeg() 读取，
 *            内部已减去上电时记录的零位偏置
 *
 * @author  LQ_012
 * @date    2026-07-27
 *******************************************************************************/

#include "include.h"

/* ============================================================
 *  返回码
 *  用于 MID_IMU_Init / MID_IMU_SFLPInit 的结果返回，
 *  业务层判断时只需用 MID_IMU_OK / MID_IMU_ERR 比较
 *  无需关心具体的设备 ID
 * ============================================================ */
#define MID_IMU_OK      (0U)   /* 初始化成功 / 读取成功 */
#define MID_IMU_ERR     (1U)   /* 初始化失败 / 读取失败 */

/* ============================================================
 *  硬件 / 业务层 API
 * ============================================================ */

/* 初始化 LSM6DSV16X 传感器（SPI 接口）
 * @return  MID_IMU_OK  成功（设备 ID 匹配）
 *           MID_IMU_ERR 失败（SPI 通信或设备 ID 不匹配）
 * @note    必须在 main() 中先于其他陀螺仪 API 调用一次
 */
uint8_t MID_IMU_Init(void);

/* 读取陀螺仪 Z 轴原始角速度
 * @return  角速度 (deg/s)，未做零偏校正
 *          = 原始 LSB * 1/14.286（对应 ±2000dps 量程）
 * @note    业务层使用时需自行减去 g_param.gyro_offset
 *          调用频率：5ms 一次（与 5ms 调度器匹配）
 */
float MID_IMU_ReadGyroZRaw(void);

/* 陀螺仪 Z 轴零偏自动校准
 * @note    调用前确保小车处于静止状态，校准期间内部会：
 *          - 暂时保存并关闭全部 PID，强制停车
 *          - 连续采样 200 次取平均写入 g_param.gyro_offset
 *          - 复位 PID 历史状态（防旧积分把新偏差"锁住"）
 *          - 恢复三环使能与目标速度
 *          - 蜂鸣器提示：开始 1 声短"滴"，结束 2 声短"滴滴"
 *          耗时约 1 秒，期间主循环无法被 __WFI 唤醒
 *  典型调用位置：
 *          1. main() 中 IMU 初始化完成后（上电自动校准）
 */
void MID_IMU_CalibrateGyro(void);

/* 初始化 SFLP 6 轴姿态融合 + 记录初始 yaw 零位
 * @return  MID_IMU_OK  成功
 *           MID_IMU_ERR 失败（SFLP 模块未使能或读不到四元数）
 * @note    必须在 MID_IMU_Init() 成功之后调用一次
 *          内部行为：开启 SFLP + 读一次四元数 -> 转 yaw -> 记为 s_yaw_offset
 *          之后 MID_IMU_ReadYawDeg() 自动减掉该零位
 *          SFLP 硬件需约 1s 稳定期，本函数内部已等 50ms；
 *          建议 main() 中后续 delay_ms(1000) 给 SFLP 充分启动
 */
uint8_t MID_IMU_SFLPInitAndCalibrateYaw(void);

/* 读取融合后的偏航角 yaw（已减去上电零位）
 * @return  偏航角 (deg)，范围 -180 ~ +180
 *          每次调用都会从 SFLP 读四元数并实时刷新
 * @note    调用频率：5ms 一次
 *          如果 SFLP 未初始化完成，返回 0.0f
 */
float MID_IMU_ReadYawDeg(void);

#endif
