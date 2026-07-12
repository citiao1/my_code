#ifndef __LQ_LSM6DSR_HARD_H
#define __LQ_LSM6DSR_HARD_H

#include "include.h"


#define HARDWARE_SPI         // 硬件SPI方式读取LSM6DSR六轴陀螺仪
// #define HARDWARE_IIC           // 硬件IIC方式读取LSM6DSR六轴陀螺仪


#define LSM6DSR_ADDR_7BIT      0x6A   // ADO=0 ，若ADO=1，则为0x6B 与SPI时地址相同

#define LSM6DSR_CK_HARD_IO     P43
#define LSM6DSR_DI_HARD_IO     P41
#define LSM6DSR_DO_HARD_IO     P42
#define LSM6DSR_CS_HARD_IO     P40

#define LSM6DSR_CK_STA(n)   GPIO_Set_Bit(LSM6DSR_CK_HARD_IO,n)
// #define LSM6DSR_DI_STA(n)   GPIO_Set_Bit(LSM6DSR_DI_HARD_IO,n)  // 硬件自动完成
// #define LSM6DSR_DO_STA(n)   GPIO_Set_Bit(LSM6DSR_DO_HARD_IO,n)  // 硬件自动完成
#define LSM6DSR_CS_STA(n)   GPIO_Set_Bit(LSM6DSR_CS_HARD_IO,n)




/*
 * 使用 I2C 通信方式时，七位地址其中 SDA[0] 与 AD0 脚有关，AD0 接低电平则为 0，AD0 接高电平则为 1
 * 其中SDA[6:1]这六位的值为110101，对应基础设备地址为0x35
 * 不同接线方式对应不同设备地址，可以凭借此方法，通过 I2C 总线级联两个相同的设备
 */

// include 中已经加载过 //#include "LQ_LSM6DSR_Soft.h" 包含有同样的，寄存器名称，此处注释掉，单独使用时可打开
typedef enum
{
    HLSM6DSR_AD0_PIN     = P4_2,     /* IIC  通信时ADO 所用引脚来确定设备ID */
    HLSM6DSR_AD0         = 0,        /* 根据 ADO 引脚电平，选择是 0 还是 1*/
    
    HLSM6DSR_BASE_ADDR   = 0x35,
    HLSM6DSR_ADDR        = (HLSM6DSR_BASE_ADDR << 1) + HLSM6DSR_AD0,  /* IIC通信设备 写地址 读则加一 */
    
    HLSM6DSR_DRV_ID      = 0x6B,     /* 设备ID */
    HLSM6DSR_WHO_AM      = 0x0F,     /* 设备ID寄存器 */
    
    HLSM6DSR_FUNC_CFG    = 0X01,     /* 控制寄存器 */
    HLSM6DSR_INT1_CTRL   = 0X0D,
    HLSM6DSR_INT2_CTRL   = 0X0E,
    
    /* 加速度计控制寄存器CTRL1_XL (r/w) 
     bit[1 : 0] -> 一级数字滤波输出. 1 -> LPF2第二级滤波输出
     bit[2:3] : 加速度计量程选择, 默认为 00 -> ±2g, 01 -> ±16g, 10 -> ±4g, 11 -> ±8g
    */
    HLSM6DSR_CTRL1_XL    = 0X10,
    HLSM6DSR_CTRL2_G     = 0X11,
    HLSM6DSR_CTRL3_C     = 0X12,
    HLSM6DSR_CTRL4_C     = 0X13,
    HLSM6DSR_CTRL5_C     = 0X14,
    HLSM6DSR_CTRL6_C     = 0X15,
    HLSM6DSR_CTRL7_G     = 0X16,
    HLSM6DSR_CTRL8_XL    = 0X17,
    HLSM6DSR_CTRL9_XL    = 0X18,
    HLSM6DSR_CTRL10_C    = 0X19,
    
    HLSM6DSR_STATUS_REG  = 0X1E,
    
    HLSM6DSR_OUT_TEMP_L  = 0X20,
    HLSM6DSR_OUT_TEMP_H  = 0X21,
    
    HLSM6DSR_OUTX_L_GYRO = 0X22,
    HLSM6DSR_OUTX_H_GYRO = 0X23,
    HLSM6DSR_OUTY_L_GYRO = 0X24,
    HLSM6DSR_OUTY_H_GYRO = 0X25,
    HLSM6DSR_OUTZ_L_GYRO = 0X26,
    HLSM6DSR_OUTZ_H_GYRO = 0X27,
    
    HLSM6DSR_OUTX_L_ACC  = 0X28,
    HLSM6DSR_OUTX_H_ACC  = 0X29,
    HLSM6DSR_OUTY_L_ACC  = 0X2A,
    HLSM6DSR_OUTY_H_ACC  = 0X2B,
    HLSM6DSR_OUTZ_L_ACC  = 0X2C,
    HLSM6DSR_OUTZ_H_ACC  = 0X2D,
    
    HLSM6DSR_I3C_BUS_AVB = 0x62,
    HPROPERTY_ENABLE     = 1U,
    HPROPERTY_DISABLE    = 0U,
    
}LSM6DSR_Reg_emu;

/*
 * CTRL1_XL寄存器中的 FS_XL[1:0]两位用来配置加速度计的最大量程选择
 量程(g)-----------------应用场景---------------------------------说明-------------------------------------
 ±2g 》16384 健康监测(如步行、跑步分析)        》|适用于人体运动检测，能够精确捕捉较小的加速度变化。
 ±4g 》8192  消费电子产品(如智能手机、平板电脑)》|能够覆盖日常使用中的大多数动作，包括倾斜、摇晃等。
 ±8g 》4096  运动设备(如智能手表、健身追踪器)  》|更广泛的运动范围，适合于更剧烈的体育活动，如篮球、足球等。
 ±16g》2048  汽车碰撞检测、工业震动监测        》|适用于高冲击环境，如汽车安全气囊触发机制，或者重型机械的振动分析。

 * CTRL1_XL寄存器中的ODR_XL[3:0]四位用来配置加速度计的数据输出速率
 */
/* CTRL1_XL[3:2] 加速度计 量程 */

typedef enum
{
 
    ACC_LPF1_SEL_0 = 0x00,   /* CTRL1_XL[1]  0:一级数字滤波输出. 1:LPF2第二级滤波输出 */
    ACC_LPF1_SEL_1 = 0x02,
    
    /* CTRL1_XL[3:2] 加速度计 量程 */
    ACC_FS_XL_2G  = 0x00,   // FS_XL[1:0]=00 
    ACC_FS_XL_16G = 0x04,   // FS_XL[1:0]=01
    ACC_FS_XL_4G  = 0x08,   // FS_XL[1:0]=10
    ACC_FS_XL_8G  = 0x0C,   // FS_XL[1:0]=11
    

    BW0XL_1_5KHz = 0x00,    /* 模块模拟链带宽 CTRL1_XL[0]  0:1.5KHz, 1:400Hz */
    BW0XL_400Hz  = 0x01,
    
    
    /* CTRL1_XL[7:4]加速度输出 速度；CTRL2_G[7:4]加速度输出 速度 */
    RATE_OUT_0Hz  = 0x00,   // ODR_XL[3:0] = 0000 关闭输出 
    RATE_12_5Hz   = 0x10,   // ODR_XL[3:0] = 0100 12.5Hz输出 
    RATE_26Hz     = 0x20,
    RATE_52Hz     = 0x30,
    RATE_104Hz    = 0x40,
    RATE_208Hz    = 0x50,
    RATE_416Hz    = 0x60,
    RATE_833Hz    = 0x70,
    RATE_1_66kHz  = 0x80,
    RATE_3_33kHz  = 0x90,
    RATE_6_66kHz  = 0xA0,
     
    /* 加速度计带宽选择 */
    ACC_LOW_PASS_ODR_50  = 0x88,     // 低通滤波器
    ACC_LOW_PASS_ODR_100 = 0xA8,
    ACC_LOW_PASS_ODR_9	 = 0xC8,

}lqLSM6DSR_SetReg;



#ifdef HARDWARE_IIC

void LQ_HARD_IIC_LSM60DSR_Init(void);     // 硬件IIC初始化
void Test_LSM6DSR_Hard(void);             // 测试硬件IIC读取LSM6DSR六轴陀螺仪

#elif defined HARDWARE_SPI

void LQ_HARD_SPI_LSM60DSR_Init(void);     // 硬件SPI初始化
void LSM6DSR_Read_Data(int16 *ax, int16 *ay, int16 *az, int16 *gx, int16 *gy, int16 *gz);           // 测试硬件SPI读取LSM6DSR六轴陀螺仪
int Get_Yaw_Rate();
#endif


#endif
