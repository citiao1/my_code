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
#ifndef __LQ_SPI_GYRO_H_
#define __LQ_SPI_GYRO_H_

#include "include.h"

#define USE_SOFT_SPI    //通过宏选择使用硬件SPI还是软件SPI
//#define USE_HAL_SPI  








#ifdef USE_SOFT_SPI
#include "LQ_Soft_SPI.h"

#else

//#include "LQ_HAL_SPI.h"
//后续完善

// 硬件SPI1 引脚选择，请根据实际硬件修改
// #define GYRO_SPI        SPI1

 #define GYRO_SCK_PIN    SPI1_1_SCK_PB3      //SCL/SPC
 #define GYRO_SDI_PIN    SPI1_1_MOSI_PB5     //
 #define GYRO_SDO_PIN    SPI1_1_MISO_PB4
 #define GYRO_CS_PIN     SPI_CS

#endif

 #define GYRO_CS_H       SPI_CS=1
 #define GYRO_CS_L       SPI_CS=0

enum//SPI通信陀螺仪类型
{
    error = 0,
    SPI_ICM20602,
    SPI_ICM20689,
    SPI_MPU9250,
    SPI_ICM42605,
    SPI_ICM42688,
    SPI_LSM6DSR,
    SPI_LQ9AGMV,
}GyroType;


//***************************************
// 定义ICM20602内部地址
//***************************************
//MPU6500的内部寄存器
#define ICM_SELF_TESTX_REG      0X0D    //自检寄存器X
#define ICM_SELF_TESTY_REG      0X0E    //自检寄存器Y
#define ICM_SELF_TESTZ_REG      0X0F    //自检寄存器Z
#define ICM_SELF_TESTA_REG      0X10    //自检寄存器A
#define ICM_SAMPLE_RATE_REG     0X19    //采样频率分频器
#define ICM_CFG_REG             0X1A    //配置寄存器
#define ICM_GYRO_CFG_REG        0X1B    //陀螺仪配置寄存器
#define ICM_ACCEL_CFG_REG       0X1C    //加速度计配置寄存器
#define ICM_MOTION_DET_REG      0X1F    //运动检测阀值设置寄存器
#define ICM_FIFO_EN_REG         0X23    //FIFO使能寄存器

#define ICM_I2CMST_STA_REG      0X36    //IIC主机状态寄存器
#define ICM_INTBP_CFG_REG       0X37    //中断/旁路设置寄存器
#define ICM_INT_EN_REG          0X38    //中断使能寄存器
#define ICM_INT_STA_REG         0X3A    //中断状态寄存器

#define ICM_I2CSLV0_DO_REG      0X63    //IIC从机0数据寄存器
#define ICM_I2CSLV1_DO_REG      0X64    //IIC从机1数据寄存器
#define ICM_I2CSLV2_DO_REG      0X65    //IIC从机2数据寄存器
#define ICM_I2CSLV3_DO_REG      0X66    //IIC从机3数据寄存器

#define ICM_I2CMST_DELAY_REG    0X67    //IIC主机延时管理寄存器
#define ICM_SIGPATH_RST_REG     0X68    //信号通道复位寄存器
#define ICM_MDETECT_CTRL_REG    0X69    //运动检测控制寄存器
#define ICM_USER_CTRL_REG       0X6A    //用户控制寄存器
#define ICM_PWR_MGMT1_REG       0X6B    //电源管理寄存器1
#define ICM_PWR_MGMT2_REG       0X6C    //电源管理寄存器2
#define ICM_FIFO_CNTH_REG       0X72    //FIFO计数寄存器高八位
#define ICM_FIFO_CNTL_REG       0X73    //FIFO计数寄存器低八位
#define ICM_FIFO_RW_REG         0X74    //FIFO读写寄存器
#define WHO_AM_I                0X75    //器件ID寄存器

#define ICM_ACCEL_XOUTH_REG     0X3B    //加速度值,X轴高8位寄存器

#define ICM_ACCEL_DATA_X1       0X1F    //加速度值,X轴高8位寄存器  ICM42605
#define ICM_ACCEL_DATA_X0       0X20    //加速度值,X轴低8位寄存器
#define ICM_ACCEL_DATA_Y1       0X21    //加速度值,Y轴高8位寄存器
#define ICM_ACCEL_DATA_Y0       0X22    //加速度值,Y轴低8位寄存器
#define ICM_ACCEL_DATA_Z1       0X23    //加速度值,Z轴高8位寄存器
#define ICM_ACCEL_DATA_Z0       0X24    //加速度值,Z轴低8位寄存器

#define ICM_GYRO_DATA_X1        0X25    //陀螺仪值,X轴高8位寄存器
#define ICM_GYRO_DATA_X0        0X26    //陀螺仪值,X轴低8位寄存器
#define ICM_GYRO_DATA_Y1        0X27    //陀螺仪值,Y轴高8位寄存器
#define ICM_GYRO_DATA_Y0        0X28    //陀螺仪值,Y轴低8位寄存器
#define ICM_GYRO_DATA_Z1        0X29    //陀螺仪值,Z轴高8位寄存器
#define ICM_GYRO_DATA_Z0        0X2A    //陀螺仪值,Z轴低8位寄存器

//LSM6D陀螺仪寄存器定义
#define DRV_ID_LSM6D            0x6B    //设备ID
#define WHO_AM_LSM6D            0x0F    //设备ID寄存器

// 定义LSM6DSR_FUNC_CFG寄存器地址
#define LSM6DSR_FUNC_CFG        0X01
#define LSM6DSR_INT1_CTRL       0X0D
#define LSM6DSR_INT2_CTRL       0X0E

#define LSM6DSR_CTRL1_XL        0X10    // 加速度计控制寄存器1 (r/w) bit1:0:一级数字滤波输出。1:LPF2第二级滤波输出 bit[2:3]:加速度计量程选择，默认为00:±2g 01:±16g 10:±4g 11:±8g
#define LSM6DSR_CTRL2_G         0X11    // 加速度计控制寄存器2
#define LSM6DSR_CTRL3_C         0X12    // 陀螺仪控制寄存器3
#define LSM6DSR_CTRL4_C         0X13    // 陀螺仪控制寄存器4
#define LSM6DSR_CTRL5_C         0X14    // 陀螺仪控制寄存器5
#define LSM6DSR_CTRL6_C         0X15    // 陀螺仪控制寄存器6
#define LSM6DSR_CTRL7_G         0X16    // 陀螺仪控制寄存器7
#define LSM6DSR_CTRL8_XL        0X17    // 加速度计控制寄存器8
#define LSM6DSR_CTRL9_XL        0X18    // 加速度计控制寄存器9
#define LSM6DSR_CTRL10_C        0X19    // 陀螺仪控制寄存器10

#define LSM6DSR_STATUS_REG      0X1E    // 状态寄存器

#define LSM6DSR_OUT_TEMP_L      0X20    // 温度寄存器低字节
#define LSM6DSR_OUT_TEMP_H      0X21    // 温度寄存器高字节


// 定义陀螺仪数据寄存器地址
#define LSM6DSR_OUTX_L_GYRO     0X22    // 陀螺仪X轴低字节
#define LSM6DSR_OUTX_H_GYRO     0X23    // 陀螺仪X轴高字节
#define LSM6DSR_OUTY_L_GYRO     0X24    // 陀螺仪Y轴低字节
#define LSM6DSR_OUTY_H_GYRO     0X25    // 陀螺仪Y轴高字节
#define LSM6DSR_OUTZ_L_GYRO     0X26    // 陀螺仪Z轴低字节
#define LSM6DSR_OUTZ_H_GYRO     0X27    // 陀螺仪Z轴高字节

// 定义加速度计数据寄存器地址
#define LSM6DSR_OUTX_L_ACC      0X28    // 加速度计X轴低字节
#define LSM6DSR_OUTX_H_ACC      0X29    // 加速度计X轴高字节
#define LSM6DSR_OUTY_L_ACC      0X2A    // 加速度计Y轴低字节
#define LSM6DSR_OUTY_H_ACC      0X2B    // 加速度计Y轴高字节
#define LSM6DSR_OUTZ_L_ACC      0X2C    // 加速度计Z轴低字节
#define LSM6DSR_OUTZ_H_ACC      0X2D    // 加速度计Z轴高字节

#define LSM6DSR_I3C_BUS_AVB     0x62    // LSM6DSR I3C总线可用
#define PROPERTY_ENABLE         (1U)    // 使能
#define PROPERTY_DISABLE        (0U)    // 失能

#define Mag_ID                  0x3D    //磁力计ID


unsigned char Gyro_Get_Raw_data_SPI(short *ax,short *ay,short *az,short *gx,short *gy,short *gz);


#endif

