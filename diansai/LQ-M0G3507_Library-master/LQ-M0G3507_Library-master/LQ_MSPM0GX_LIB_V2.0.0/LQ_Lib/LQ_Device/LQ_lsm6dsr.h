/*******************************************************************************
 * @file                LQ_lsm6dsr.h
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
#ifndef __LQ_LSM6DSR_H__
#define __LQ_LSM6DSR_H__

#include "include.h"

//-----------------LSM6DSR 六轴陀螺仪引脚控制宏定义-----------------

#define LSM6DSR_SCK					GPIO_Pin_A_12	/* SCK  引脚 */
#define LSM6DSR_SDA					GPIO_Pin_A_14	/* MOSI 引脚 */
#define LSM6DSR_SDO					GPIO_Pin_A_13	/* MISO 引脚 */
#define LSM6DSR_CS					GPIO_Pin_A_2	/* CS   引脚 */

#define LSM6DSR_SCL_Clr() 			LQ_GPIO_WritePin(LSM6DSR_SCK, 0)	// SCL
#define LSM6DSR_SCL_Set() 			LQ_GPIO_WritePin(LSM6DSR_SCK, 1)

#define LSM6DSR_SDA_Clr() 			LQ_GPIO_WritePin(LSM6DSR_SDA, 0)	// SDA(MISO)
#define LSM6DSR_SDA_Set() 			LQ_GPIO_WritePin(LSM6DSR_SDA, 1)

#define LSM6DSR_CS_Clr()   			LQ_GPIO_WritePin(LSM6DSR_CS, 0)		// CS
#define LSM6DSR_CS_Set()   			LQ_GPIO_WritePin(LSM6DSR_CS, 1)

#define LSM6DSR_SDO_READ	 		LQ_GPIO_ReadPin(LSM6DSR_SDO)		// SDO(MOSI)

//-----------------LSM6DSR 六轴陀螺仪寄存器宏定义-----------------

/* AD0 接低电平则为 0，AD0 接高电平则为 1
 * 不同接线方式对应不同设备地址，可以凭借此方法，通过 IIC 总线级联两个相同的设备
 * */
#define AD0                     0

#define BASE_ADDR               0x35
// #define LSM6DSR_ADDR ((((BASE_ADDR)<<1)+AD0)<<1)    // IIC 通信设备 写地址 读则加一
#define LSM6DSR_ADDR            (((BASE_ADDR)<<1)+AD0)    // IIC 通信设备 写地址 读则加一

#define DRV_ID_LSM6D            0x6B    // 设备ID
#define WHO_AM_LSM6D            0x0F    // 设备ID寄存器

#define LSM6DSR_FUNC_CFG        0X01    // 控制寄存器
#define LSM6DSR_INT1_CTRL       0X0D
#define LSM6DSR_INT2_CTRL       0X0E

#define LSM6DSR_CTRL1_XL        0X10    // 加速度计控制寄存器1 (r/w) bit1:0:一级数字滤波输出。1:LPF2第二级滤波输出 bit[2:3]:加速度计量程选择，默认为00:±2g 01:±16g 10:±4g 11:±8g
#define LSM6DSR_CTRL2_G         0X11
#define LSM6DSR_CTRL3_C         0X12
#define LSM6DSR_CTRL4_C         0X13
#define LSM6DSR_CTRL5_C         0X14
#define LSM6DSR_CTRL6_C         0X15
#define LSM6DSR_CTRL7_G         0X16
#define LSM6DSR_CTRL8_XL        0X17
#define LSM6DSR_CTRL9_XL        0X18
#define LSM6DSR_CTRL10_C        0X19

#define LSM6DSR_STATUS_REG      0X1E
        
#define LSM6DSR_OUT_TEMP_L      0X20
#define LSM6DSR_OUT_TEMP_H      0X21

#define LSM6DSR_OUTX_L_GYRO     0X22
#define LSM6DSR_OUTX_H_GYRO     0X23
#define LSM6DSR_OUTY_L_GYRO     0X24
#define LSM6DSR_OUTY_H_GYRO     0X25
#define LSM6DSR_OUTZ_L_GYRO     0X26
#define LSM6DSR_OUTZ_H_GYRO     0X27

#define LSM6DSR_OUTX_L_ACC      0X28
#define LSM6DSR_OUTX_H_ACC      0X29
#define LSM6DSR_OUTY_L_ACC      0X2A
#define LSM6DSR_OUTY_H_ACC      0X2B
#define LSM6DSR_OUTZ_L_ACC      0X2C
#define LSM6DSR_OUTZ_H_ACC      0X2D

#define LSM6DSR_I3C_BUS_AVB     0x62
#define PROPERTY_ENABLE         (1U)
#define PROPERTY_DISABLE        (0U)

#define Mag_ID                  0x3D    // 挂载地磁ID

//-----------------LSM6DSR 六轴陀螺仪函数定义-----------------

/*! @brief	LSM6DSR 初始化 */
void LQ_LSM6DSR_Init(void);

/*! @brief	获取 LSM6DSR 的 ID 值 */
unsigned char LQ_LSM6DSR_CheckDeviceID(void);

/*! @brief	获取 LSM6DSR的 6 轴数据 */
void LQ_LSM6DSR_Read6AxisData(signed short *ax, signed short *ay, signed short *az, signed short *gx, signed short *gy, signed short *gz);

#endif
