#ifndef _LQ_LSM6DSR_H
#define _LQ_LSM6DSR_H

#include "include.h"


//-----------------LSM6DSR 六轴陀螺仪最终数据-----------------
extern signed short LQ_LSM6DSR_Acc_X, LQ_LSM6DSR_Acc_Y, LQ_LSM6DSR_Acc_Z;
extern signed short LQ_LSM6DSR_Gyro_X, LQ_LSM6DSR_Gyro_Y, LQ_LSM6DSR_Gyro_Z;


//-----------------LSM6DSR 六轴陀螺仪引脚控制宏定义-----------------

#define LSM6DSR_SCL_Clr() 	DL_GPIO_clearPins(LSM6DSR_PORT,LSM6DSR_LSM6DSR_SCL_PIN)//SCL
#define LSM6DSR_SCL_Set() 	DL_GPIO_setPins(LSM6DSR_PORT,LSM6DSR_LSM6DSR_SCL_PIN)

#define LSM6DSR_SDA_Clr() 	DL_GPIO_clearPins(LSM6DSR_PORT,LSM6DSR_LSM6DSR_MISO_PIN)//SDA(MISO)
#define LSM6DSR_SDA_Set() 	DL_GPIO_setPins(LSM6DSR_PORT,LSM6DSR_LSM6DSR_MISO_PIN)

#define LSM6DSR_CS_Clr()   DL_GPIO_clearPins(LSM6DSR_PORT,LSM6DSR_LSM6DSR_CS_PIN)//CS
#define LSM6DSR_CS_Set()   DL_GPIO_setPins(LSM6DSR_PORT,LSM6DSR_LSM6DSR_CS_PIN)

#define LSM6DSR_SDO_READ	 LQ_GPIO_readPins(LSM6DSR_PORT, LSM6DSR_LSM6DSR_MOSI_PIN)//SDO(MOSI)



void LQ_LSM6DSR_SPI_ReadWriteBytes(unsigned char *lqbuff, unsigned int len);
void LQ_LSM6DSR_SPI_ReadData(unsigned char reg, unsigned int len, unsigned char* buf);
void LQ_LSM6DSR_SPI_WriteData(unsigned char reg, unsigned char value);

/* AD0 接低电平则为 0，AD0 接高电平则为 1
 * 不同接线方式对应不同设备地址，可以凭借此方法，通过 IIC 总线级联两个相同的设备
 * */
#define AD0                     0

#define BASE_ADDR               0x35
#define LSM6DSR_ADDR ((((BASE_ADDR)<<1)+AD0)<<1)    // IIC 通信设备 写地址 读则加一

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



unsigned char LQ_LSM6DSR_GetDeviceID(unsigned char reg);
void LQ_LSM6DSR_WriteRegister(unsigned char reg,unsigned char dat);
unsigned char LQ_LSM6DSR_ReadRegister(unsigned char reg);
void LQ_LSM6DSR_ReadMultipleRegisters(unsigned char reg, unsigned char* buf, unsigned char len);
unsigned char LQ_LSM6DSR_CheckDeviceID(void);
void LQ_LSM6DSR_Init(void);
void LQ_LSM6DSR_Read6AxisData(signed short *ax, signed short *ay, signed short *az, signed short *gx, signed short *gy, signed short *gz);

void LQ_Test_LSM6DSR();

#endif


