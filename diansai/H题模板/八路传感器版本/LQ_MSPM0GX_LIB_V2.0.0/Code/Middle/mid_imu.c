
/*******************************************************************************
 * @file    mid_imu.c
 * @brief   IMU / 陀螺仪中间层封装（Middle Layer）
 *
 * @note    本文件封装所有与 LSM6DSV16X 硬件打交道的代码，外部统一通过
 *          mid_imu.h 调用；chassis / scheduler / main 等上层文件不需要
 *          include 任何 LQ_lsm6dsv16x.h 相关头。
 *
 *          API 列表：
 *          - MID_IMU_Init                       初始化 LSM6DSV16X
 *          - MID_IMU_ReadGyroZRaw               读 Z 轴原始角速度 (deg/s)
 *          - MID_IMU_CalibrateGyro              静止校准，写入 g_param.gyro_offset
 *          - MID_IMU_SFLPInitAndCalibrateYaw    启动 SFLP + 记录 yaw 零位
 *          - MID_IMU_ReadYawDeg                 读零位校正后的偏航角 (deg)
 *
 *          LSB 换算系数 GYRO_LSB_TO_DPS：
 *          - LSM6DSV16X 量程 ±2000dps，sensitivity = 70 mdps/LSB
 *          - 1 LSB = 0.07 dps = 1/14.286 dps
 *          - 角速度零偏通过 g_param.gyro_offset 暴露给业务层
 *
 * @author  LQ_012
 * @date    2026-07-27
 *******************************************************************************/

#include "mid_imu.h"
#include "mid_chassis.h"        /* MID_Chassis_Init, MID_Chassis_ResetPID */
#include "mid_motor.h"          /* MID_Motor_Stop                     */
#include "mid_beep.h"           /* MID_Beep_On/Off                    */
#include "mid_pid.h"            /* g_param 全局参数结构               */
#include "LQ_lsm6dsv16x.h"      /* 唯一允许的硬件头文件，仅本文件 include */

/* ============================================================
 *  常量
 * ============================================================ */

/* LSM6DSV16X 原始 LSB -> deg/s 的换算系数（与量程 ±2000dps 配套） */
#define GYRO_LSB_TO_DPS          (1.0f / 14.286f)

/* 校准相关参数：
 * - GYRO_CALIB_SAMPLES     平均采样次数（200 次 × 5ms ≈ 1s 静止采样）
 * - GYRO_CALIB_DELAY_MS    单次采样间隔（ms），给 CPU / 中断留时间
 * - GYRO_CALIB_SETTLE_MS   校准前稳定延时（消除上次运动残留）
 */
#define GYRO_CALIB_SAMPLES       (200u)
#define GYRO_CALIB_DELAY_MS      (5u)
#define GYRO_CALIB_SETTLE_MS     (50u)

/* SFLP 启动后等待稳定时间（硬件约需 1s，本函数给 50ms，
   剩余时间由 main() 启动后 delay_ms 补足） */
#define SFLP_INIT_SETTLE_MS      (50u)

/* ============================================================
 *  内部状态（本文件私有，业务层不可见）
 * ============================================================ */

/* SFLP 零位偏置：main() 调用 MID_IMU_SFLPInitAndCalibrateYaw() 时记录 */
static float s_yaw_offset = 0.0f;

/* SFLP 是否已成功初始化：未初始化时 yaw 返回 0.0f */
static uint8_t s_sflp_ready = 0U;

/* ============================================================
 *  对外 API
 * ============================================================ */

/* 初始化 LSM6DSV16X（含 WHO_AM_I 校验）
 * 详细说明：mid_imu.h
 */
uint8_t MID_IMU_Init(void)
{
    /* LQ_LSM6DSV16X_Init() 内部会读 WHO_AM_I 寄存器，正常返回值 0x70；
     * 这里只判断 ID 是否匹配，匹配返回 OK / 否则 ERR
     */
    uint8_t id = LQ_LSM6DSV16X_Init();
    if (id == 0x70) {
        return MID_IMU_OK;
    }
    return MID_IMU_ERR;
}

/* 读陀螺仪 Z 轴原始角速度
 * 详细说明：mid_imu.h
 *
 * @note  返回值为原始值，**未减零偏**。上层如需校准值请自行
 *        gyro - g_param.gyro_offset
 *        （YawStep 内部已自动减零偏，这里保持接口最小化）
 */
float MID_IMU_ReadGyroZRaw(void)
{
    int16_t gx = 0, gy = 0, gz = 0;
    Lsm6dsv16x_Get_RawGyro(&gx, &gy, &gz);
    return -(float)gz * GYRO_LSB_TO_DPS;
}

/* 陀螺仪 Z 轴静止校准
 * 详细说明：mid_imu.h
 *
 * @note  调用前应保证小车已完全静止（被夹具固定 / 抬离地面）。
 *        流程：保存当前使能 -> 关三环 + 速度清零 + 停电机
 *           -> 稳定延时 -> 蜂鸣提示音 1 -> 采集 N 次求平均
 *           -> 写入 g_param.gyro_offset -> 复位 PID -> 恢复使能
 *           -> 蜂鸣提示音 2
 */
void MID_IMU_CalibrateGyro(void)
{
    /* 1) 临时关三环 + 直行速度清零，防止校准过程中产生意外 PWM
     *    保存原值，校准结束后恢复，给上层"无感"体验
     */
    uint8_t saved_pos    = g_param.enable_pos;
    uint8_t saved_yaw    = g_param.enable_yaw;
    uint8_t saved_speed  = g_param.enable_speed;
    float   saved_vx     = g_param.chassis_vx;
    g_param.enable_pos    = 0;
    g_param.enable_yaw    = 0;
    g_param.enable_speed  = 0;
    g_param.chassis_vx    = 0.0f;

    /* 2) 立即停电机，保证 PID 输出 0 = 实际不转 */
    MID_Motor_Stop();

    /* 3) 50ms 稳定延时（消除上次动作残留 / SPI 通信稳定） */
    delay_ms(GYRO_CALIB_SETTLE_MS);

    /* 4) 校准开始提示音：1 声"哔" */
    MID_Beep_On();
    delay_ms(80);
    MID_Beep_Off();

    /* 5) 连续 N 次读 Z 轴原始数据求平均，换算成 deg/s 写入零偏
     *    sum 累加 LSB，最后再乘 GYRO_LSB_TO_DPS（节省中间步骤） */
    int32_t sum = 0;
    for (uint16_t i = 0; i < GYRO_CALIB_SAMPLES; i++) {
        int16_t gx = 0, gy = 0, gz = 0;
        Lsm6dsv16x_Get_RawGyro(&gx, &gy, &gz);
        sum += -gz;
        delay_ms(GYRO_CALIB_DELAY_MS);
    }
    g_param.gyro_offset = ((float)sum / (float)GYRO_CALIB_SAMPLES) * GYRO_LSB_TO_DPS;

    /* 6) 复位底盘 PID 历史（清旧积分/微分） */
    MID_Chassis_Init();

    /* 7) 恢复之前保存的使能和直行速度 */
    g_param.chassis_vx   = saved_vx;
    g_param.enable_pos   = saved_pos;
    g_param.enable_yaw   = saved_yaw;
    g_param.enable_speed = saved_speed;

    /* 8) 校准完成提示音：2 声"哔哔" */
    MID_Beep_On();
    delay_ms(80);
    MID_Beep_Off();
    delay_ms(80);
    MID_Beep_On();
    delay_ms(80);
    MID_Beep_Off();
}

/* 初始化 SFLP 6 轴融合 + 记录初始 yaw 零位
 * 详细说明：mid_imu.h
 *
 * @retval MID_IMU_OK   成功（s_sflp_ready = 1）
 * @retval MID_IMU_ERR  失败（s_sflp_ready = 0，yaw 读数返回 0）
 */
uint8_t MID_IMU_SFLPInitAndCalibrateYaw(void)
{
    /* 1) 启动 SFLP 滤波器（硬件寄存器配置） */
    if (LQ_LSM6DSV16X_SFLP_Init() != 0) {
        s_sflp_ready = 0U;
        return MID_IMU_ERR;
    }

    /* 2) 等 SFLP 收敛（这里给 50ms，main 启动时再 delay 补足） */
    delay_ms(SFLP_INIT_SETTLE_MS);

    /* 3) 读一次四元数 -> 转 yaw -> 记录为零位偏置 */
    int16_t q[4];
    if (Lsm6dsv16x_GetSflpQuat(q) == 0) {
        s_yaw_offset = Lsm6dsv16x_QuatToYawDeg(q);
        s_sflp_ready = 1U;
        return MID_IMU_OK;
    }

    /* 读四元数失败：标记未就绪，yaw 后续返回 0 */
    s_sflp_ready = 0U;
    s_yaw_offset = 0.0f;
    return MID_IMU_ERR;
}

/* 读零位校正后的偏航角（deg）
 * 详细说明：mid_imu.h
 *
 * @note  SFLP 未初始化时返回 0.0f（避免上层拿到野值）
 */
float MID_IMU_ReadYawDeg(void)
{
    if (s_sflp_ready == 0U) {
        return 0.0f;
    }

    int16_t q[4];
    if (Lsm6dsv16x_GetSflpQuat(q) != 0) {
        /* 单次读失败：本次返回 0，下次重试（不做错误处理） */
        return 0.0f;
    }
    return Lsm6dsv16x_QuatToYawDeg(q) - s_yaw_offset;
}
