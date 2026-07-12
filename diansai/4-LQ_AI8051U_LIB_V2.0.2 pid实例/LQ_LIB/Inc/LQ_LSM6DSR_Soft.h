/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#ifndef __LQ_LSM6DSR_SOFT_H
#define __LQ_LSM6DSR_SOFT_H

#include "config.h"

/* 本文件包含，I2C通信方式和SPI通信方式，选择一种即可，其他一定要注释掉 */
/* 依赖驱动库 : LQ_Soft_I2C，LQ_Soft_SPI    结构体SPI和结构体I2C ====== */

#define LSM6DSR_USE_SOFT_I2C                /* 使用软件 I2C */
// #define LSM6DSR_USE_SOFT_SPI                /* 使用软件 SPI */

#if defined(LSM6DSR_USE_SOFT_I2C)   /* 使用软件 I2C 时所用引脚 */

#define LSM6DSR_I2C_SCL     P4_3
#define LSM6DSR_I2C_SDA     P4_1

#elif defined(LSM6DSR_USE_SOFT_SPI) /* 使用软件 SPI 时所用引脚 */

#define LSM6DSR_SPI_SCK     P4_3
#define LSM6DSR_SPI_MISO    P4_1
#define LSM6DSR_SPI_MOSI    P4_2
#define LSM6DSR_SPI_CS      P4_0

#endif

/*
 * 使用 I2C 通信方式时，七位地址其中 SDA[0] 与 AD0 脚有关，AD0 接低电平则为 0，AD0 接高电平则为 1
 * 其中SDA[6:1]这六位的值为110101，对应基础设备地址为0x35
 * 不同接线方式对应不同设备地址，可以凭借此方法，通过 I2C 总线级联两个相同的设备
 */
typedef enum
{
    LSM6DSR_AD0_PIN     = P4_2,     /* IIC  通信时ADO 所用引脚来确定设备ID */
    LSM6DSR_AD0         = 0,        /* 根据 ADO 引脚电平，选择是 0 还是 1*/

    LSM6DSR_BASE_ADDR   = 0x35,
    LSM6DSR_ADDR        = (LSM6DSR_BASE_ADDR << 1) + LSM6DSR_AD0,  /* IIC通信设备 写地址 读则加一 */

    LSM6DSR_DRV_ID      = 0x6B,     /* 设备ID */
    LSM6DSR_WHO_AM      = 0x0F,     /* 设备ID寄存器 */

    LSM6DSR_FUNC_CFG    = 0X01,     /* 控制寄存器 */
    LSM6DSR_INT1_CTRL   = 0X0D,
    LSM6DSR_INT2_CTRL   = 0X0E,
    
    /* 加速度计控制寄存器1 (r/w) bit1 : 0 -> 一级数字滤波输出. 1 -> LPF2第二级滤波输出
     bit[2:3] : 加速度计量程选择, 默认为 00 -> ±2g, 01 -> ±16g, 10 -> ±4g, 11 -> ±8g
    */
    LSM6DSR_CTRL1_XL    = 0X10,
    LSM6DSR_CTRL2_G     = 0X11,
    LSM6DSR_CTRL3_C     = 0X12,
    LSM6DSR_CTRL4_C     = 0X13,
    LSM6DSR_CTRL5_C     = 0X14,
    LSM6DSR_CTRL6_C     = 0X15,
    LSM6DSR_CTRL7_G     = 0X16,
    LSM6DSR_CTRL8_XL    = 0X17,
    LSM6DSR_CTRL9_XL    = 0X18,
    LSM6DSR_CTRL10_C    = 0X19,

    LSM6DSR_STATUS_REG  = 0X1E,
    
    LSM6DSR_OUT_TEMP_L  = 0X20,
    LSM6DSR_OUT_TEMP_H  = 0X21,

    LSM6DSR_OUTX_L_GYRO = 0X22,
    LSM6DSR_OUTX_H_GYRO = 0X23,
    LSM6DSR_OUTY_L_GYRO = 0X24,
    LSM6DSR_OUTY_H_GYRO = 0X25,
    LSM6DSR_OUTZ_L_GYRO = 0X26,
    LSM6DSR_OUTZ_H_GYRO = 0X27,

    LSM6DSR_OUTX_L_ACC  = 0X28,
    LSM6DSR_OUTX_H_ACC  = 0X29,
    LSM6DSR_OUTY_L_ACC  = 0X2A,
    LSM6DSR_OUTY_H_ACC  = 0X2B,
    LSM6DSR_OUTZ_L_ACC  = 0X2C,
    LSM6DSR_OUTZ_H_ACC  = 0X2D,

    LSM6DSR_I3C_BUS_AVB = 0x62,
    PROPERTY_ENABLE     = 1U,
    PROPERTY_DISABLE    = 0U,
} LSM6DSR_Reg_t;

//========================================================================
//                            外部调用函数
//========================================================================

u8   lsm6dsr_read_id(void);

void lsm6dsr_init(void);
void lsm6dsr_read_raw_data(int16 *ax, int16 *ay, int16 *az, int16 *gx, int16 *gy, int16 *gz);


#endif
