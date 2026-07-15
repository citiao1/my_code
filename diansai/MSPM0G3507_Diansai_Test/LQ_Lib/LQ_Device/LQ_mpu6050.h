/*******************************************************************************
 * @file                LQ_mpu6050.h
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_MPU6050_H__
#define __LQ_MPU6050_H__

#include "include.h"

//-----------------LSM6DSR 六轴陀螺仪引脚控制宏定义-----------------

#define MPU6050_SCL					GPIO_Pin_A_12	/* SCL 引脚 */
#define MPU6050_SDA					GPIO_Pin_A_14	/* SDA 引脚 */

#define MPU6050_ADDR                0x68

//****************************************
// 定义MPU6050内部地址
//****************************************
typedef enum
{
    MPU_SELF_TESTX_REG   = 0X0D,    // 自检寄存器X
    MPU_SELF_TESTY_REG   = 0X0E,    // 自检寄存器Y
    MPU_SELF_TESTZ_REG   = 0X0F,    // 自检寄存器z
    MPU_SELF_TESTA_REG   = 0X10,    // 自检寄存器A
    MPU_SAMPLE_RATE_REG  = 0X19,    // 采样频率分频器
    MPU_CFG_REG          = 0X1A,    // 配置寄存器
    MPU_GYRO_CFG_REG     = 0X1B,    // 陀螺仪配置寄存器
    MPU_ACCEL_CFG_REG    = 0X1C,    // 加速度计配置寄存器
    MPU_MOTION_DET_REG   = 0X1F,    // 运动检测阀值设置寄存器
    MPU_FIFO_EN_REG      = 0X23,    // FIFO使能寄存器

    MPU_I2CMST_STA_REG   = 0X36,    // IIC主机状态寄存器
    MPU_INTBP_CFG_REG    = 0X37,    // 中断/旁路设置寄存器
    MPU_INT_EN_REG       = 0X38,    // 中断使能寄存器
    MPU_INT_STA_REG      = 0X3A,    // 中断状态寄存器

    MPU_ACCEL_XOUTH_REG  = 0X3B,    // 加速度值,X轴高8位寄存器
    MPU_ACCEL_XOUTL_REG  = 0X3C,    // 加速度值,X轴低8位寄存器
    MPU_ACCEL_YOUTH_REG  = 0X3D,    // 加速度值,Y轴高8位寄存器
    MPU_ACCEL_YOUTL_REG  = 0X3E,    // 加速度值,Y轴低8位寄存器
    MPU_ACCEL_ZOUTH_REG  = 0X3F,    // 加速度值,Z轴高8位寄存器
    MPU_ACCEL_ZOUTL_REG  = 0X40,    // 加速度值,Z轴低8位寄存器

    MPU_TEMP_OUTH_REG    = 0X41,    // 温度值高八位寄存器
    MPU_TEMP_OUTL_REG    = 0X42,    // 温度值低8位寄存器

    MPU_GYRO_XOUTH_REG   = 0X43,    // 陀螺仪值,X轴高8位寄存器
    MPU_GYRO_XOUTL_REG   = 0X44,    // 陀螺仪值,X轴低8位寄存器
    MPU_GYRO_YOUTH_REG   = 0X45,    // 陀螺仪值,Y轴高8位寄存器
    MPU_GYRO_YOUTL_REG   = 0X46,    // 陀螺仪值,Y轴低8位寄存器
    MPU_GYRO_ZOUTH_REG   = 0X47,    // 陀螺仪值,Z轴高8位寄存器
    MPU_GYRO_ZOUTL_REG   = 0X48,    // 陀螺仪值,Z轴低8位寄存器

    MPU_I2CSLV0_DO_REG   = 0X63,    // IIC从机0数据寄存器
    MPU_I2CSLV1_DO_REG   = 0X64,    // IIC从机1数据寄存器
    MPU_I2CSLV2_DO_REG   = 0X65,    // IIC从机2数据寄存器
    MPU_I2CSLV3_DO_REG   = 0X66,    // IIC从机3数据寄存器

    MPU_I2CMST_DELAY_REG = 0X67,    // IIC主机延时管理寄存器
    MPU_SIGPATH_RST_REG  = 0X68,    // 信号通道复位寄存器
    MPU_MDETECT_CTRL_REG = 0X69,    // 运动检测控制寄存器
    MPU_USER_CTRL_REG    = 0X6A,    // 用户控制寄存器
    MPU_PWR_MGMT1_REG    = 0X6B,    // 电源管理寄存器1
    MPU_PWR_MGMT2_REG    = 0X6C,    // 电源管理寄存器2
    MPU_FIFO_CNTH_REG    = 0X72,    // FIFO计数寄存器高八位
    MPU_FIFO_CNTL_REG    = 0X73,    // FIFO计数寄存器低八位
    MPU_FIFO_RW_REG      = 0X74,    // FIFO读写寄存器
    WHO_AM_I             = 0X75,    // 器件ID寄存器
} LQ_MPU6050_Reg_Addr_t;

uint8_t LQ_MPU6050_Init();

uint8_t LQ_MPU6050_GetId();

uint8_t LQ_MPU6050_Get_GyroData(int16_t *gx, int16_t *gy, int16_t *gz);
uint8_t LQ_MPU6050_Get_AccData (int16_t *ax, int16_t *ay, int16_t *az);
uint8_t LQ_MPU6050_Get_RawData (int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz);

#endif
