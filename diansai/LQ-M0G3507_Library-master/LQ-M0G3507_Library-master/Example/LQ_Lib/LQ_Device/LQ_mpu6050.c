/*******************************************************************************
 * @file                LQ_mpu6050.c
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
#include "LQ_mpu6050.h"

/* 默认 MPU6050 I2C 初始化参数 */
static LQConfig_SoftI2C_InitTypeDef_t mpu6050_soft_i2c_init = {
    .scl  = MPU6050_SCL,
    .sda  = MPU6050_SDA,
    .addr = MPU6050_ADDR
};

/********************************************************************************
 * @brief   发送多个数据
 * 
 * @param   reg : 寄存器地址
 * @param   dat : 数据缓冲区
 * @param   len : 数据长度
 * 
 * @return  返回发送数据长度
 ********************************************************************************/
uint8_t LQ_MPU6050_Write_Regs(uint8_t reg, uint8_t *dat, uint16_t len)
{
    return LQ_Soft_I2C_SendBuffer(&mpu6050_soft_i2c_init, reg, dat, len);
}

/********************************************************************************
 * @brief   读取多个数据
 * 
 * @param   reg : 寄存器地址
 * @param   dat : 数据缓冲区
 * @param   len : 数据长度
 * 
 * @return  返回接收数据长度
 ********************************************************************************/
uint8_t LQ_MPU6050_Read_Regs(uint8_t reg, uint8_t *dat, uint16_t len)
{
    return LQ_Soft_I2C_RecvBuffer(&mpu6050_soft_i2c_init, reg, dat, len);
}

/********************************************************************************
 * @brief   发送一个数据
 * @param   reg : 寄存器地址
 * @param   dat : 数据
 * 
 * @return  为 1 表示设置成功，小于等于 0 表示失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Write_Reg(uint8_t reg, uint8_t dat)
{
    return LQ_Soft_I2C_SendByte(&mpu6050_soft_i2c_init, reg, dat);
}

/********************************************************************************
 * @brief   读取一个数据
 * @param   reg : 寄存器地址
 * @param   dat : 数据指针
 * 
 * @return  为 1 表示设置成功，小于等于 0 表示失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Read_Reg(uint8_t reg, uint8_t *dat)
{
    return LQ_Soft_I2C_RecvByte(&mpu6050_soft_i2c_init, reg, dat);
}

/********************************************************************************
 * @brief   设置陀螺仪测量范围
 * 
 * @param   fsr : 0 --> ±250dps    1 --> ±500dps
 *                2 --> ±1000dps   3 --> ±2000dps
 * 
 * @return  为 1 表示设置成功，小于等于 0 表示设置失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Set_Gyro_Fsr(uint8_t fsr)
{
    return LQ_MPU6050_Write_Reg(MPU_GYRO_CFG_REG, fsr << 3);
}

/********************************************************************************
 * @brief   设置加速度计测量范围
 * 
 * @param   fsr : 0 --> ±2g    1 --> ±4g
 *                2 --> ±8g    3 --> ±16g
 * 
 * @return  为 1 表示设置成功，小于等于 0 表示设置失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Set_Accel_Fsr(uint8_t fsr)
{
    return LQ_MPU6050_Write_Reg(MPU_ACCEL_CFG_REG, fsr << 3);
}

/********************************************************************************
 * @brief   设置数字低通滤波
 * 
 * @param   lpf : 数字低通滤波频率(Hz)
 * 
 * @return  为 1 表示设置成功，小于等于 0 表示设置失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Set_LPF(uint16_t lpf)
{
    uint8_t dat = 0;
    if (lpf >= 188)     { dat = 1; }
    else if (lpf >= 98) { dat = 2; }
    else if (lpf >= 42) { dat = 3; }
    else if (lpf >= 20) { dat = 4; }
    else if (lpf >= 10) { dat = 5; }
    else                { dat = 6; }
    return LQ_MPU6050_Write_Reg(MPU_CFG_REG, dat);  // 设置数字低通滤波器
}

/********************************************************************************
 * @brief   设置采样率
 * 
 * @param   rate: 4 ~ 1000(Hz)
 * 
 * @return  为 1 表示设置成功，小于等于 0 表示设置失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Set_Rate(uint16_t rate)
{
    uint8_t dat;
    if (rate > 1000) { rate = 1000; }
    if (rate < 4)    { rate = 4; }
    dat = 1000 / rate - 1;
    LQ_MPU6050_Write_Reg(MPU_SAMPLE_RATE_REG, dat); // 设置数字低通滤波器
    return LQ_MPU6050_Set_LPF(rate / 2);            // 自动设置LPF为采样率的一半
}

/********************************************************************************
 * @brief    获取温度值
 * 
 * @param    none
 * 
 * @return   温度值(扩大了100倍)
 ********************************************************************************/
int16_t LQ_MPU6050_Get_Temperature()
{
    uint8_t buf[2];
    int16_t raw;
    int32_t temp;
    const int32_t SCALE = 10000;        // 放大倍数
    const int32_t DIVISOR = 33387;      // 333.87 * 100
    const int32_t OFFSET = 21 * SCALE;  // 偏移量放大

    LQ_MPU6050_Read_Regs(MPU_TEMP_OUTH_REG, buf, 2);
    raw = (((uint16_t)buf[0] << 8) | buf[1]);
    temp = OFFSET + (raw * SCALE) / DIVISOR;
    
    return (int16_t)(temp / 100);
}

/********************************************************************************
 * @brief   获取陀螺仪值
 * 
 * @param   gx,gy,gz: 陀螺仪 x,y,z 轴的原始读数(带符号)
 * 
 * @return  为 2 表示读取成功，其他则失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Get_GyroData(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6], res;
    res = LQ_MPU6050_Read_Regs(MPU_GYRO_XOUTH_REG, buf, 6);
    if (res == 0)
    {
        *gx = ((uint16_t)buf[0] << 8) | buf[1];
        *gy = ((uint16_t)buf[2] << 8) | buf[3];
        *gz = ((uint16_t)buf[4] << 8) | buf[5];
    }
    return res;
}

/********************************************************************************
 * @brief   获取加速度值
 * 
 * @param   ax,ay,az: 陀螺仪 x,y,z 轴的原始读数(带符号)
 * 
 * @return  为 2 表示读取成功，其他则失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Get_AccData(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6], res;
    res = LQ_MPU6050_Read_Regs(MPU_ACCEL_XOUTH_REG, buf, 6);
    if (res == 0)
    {
        *ax = ((uint16_t)buf[0] << 8) | buf[1];
        *ay = ((uint16_t)buf[2] << 8) | buf[3];
        *az = ((uint16_t)buf[4] << 8) | buf[5];
    }
    return res;
}

/********************************************************************************
 * @brief   获取 加速度值 角速度值
 * 
 * @param   ax,ay,az: 陀螺仪 x,y,z 轴的加速度值原始读数(带符号)
 * @param   gx,gy,gz: 陀螺仪 x,y,z 轴的角速度值原始读数(带符号)
 * 
 * @return  为 2 表示读取成功，其他则失败
 ********************************************************************************/
uint8_t LQ_MPU6050_Get_RawData(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14], res;
    res = LQ_MPU6050_Read_Regs(MPU_ACCEL_XOUTH_REG, buf, 14);
    if (res == 0)
    {
        *ax = ((uint16_t)buf[0] << 8) | buf[1];
        *ay = ((uint16_t)buf[2] << 8) | buf[3];
        *az = ((uint16_t)buf[4] << 8) | buf[5];
        *gx = ((uint16_t)buf[8] << 8) | buf[9];
        *gy = ((uint16_t)buf[10] << 8) | buf[11];
        *gz = ((uint16_t)buf[12] << 8) | buf[13];
    }
    return res;
}

/********************************************************************************
 * @brief   初始化MPU6050
 * 
 * @param   none
 * 
 * @return  成功返回 0，失败返回 1
 ********************************************************************************/
uint8_t LQ_MPU6050_Init()
{
    uint8_t res;
    
    LQ_Soft_I2C_Init(&mpu6050_soft_i2c_init);

    res = 0;
    res += LQ_MPU6050_Write_Reg(MPU_PWR_MGMT1_REG, 0X80);   // 复位MPU6050
    delay_ms(100);                                          // 延时100ms
    res += LQ_MPU6050_Write_Reg(MPU_PWR_MGMT1_REG, 0X00);   // 唤醒MPU6050
    res += LQ_MPU6050_Set_Gyro_Fsr(3);                  // 陀螺仪传感器,±2000dps
    res += LQ_MPU6050_Set_Accel_Fsr(1);                 // 加速度传感器,±4g
    res += LQ_MPU6050_Set_Rate(1000);                   // 设置采样率1000Hz
    res += LQ_MPU6050_Write_Reg(MPU_CFG_REG, 0x02);         // 设置数字低通滤波器   98hz
    res += LQ_MPU6050_Write_Reg(MPU_INT_EN_REG, 0X00);      // 关闭所有中断
    res += LQ_MPU6050_Write_Reg(MPU_USER_CTRL_REG, 0X00);   // I2C主模式关闭
    res += LQ_MPU6050_Write_Reg(MPU_PWR_MGMT1_REG, 0X01);   // 设置CLKSEL,PLL X轴为参考
    res += LQ_MPU6050_Write_Reg(MPU_PWR_MGMT2_REG, 0X00);   // 加速度与陀螺仪都工作

    return 0;
}

/********************************************************************************
 * @brief   读取陀螺仪的设备ID
 * @param   dev : 自定义 I2C 相关结构体
 * @param   mod : 自定义模块相关结构体
 * @return  陀螺仪的设备ID
 * @date    2025/3/20
 ********************************************************************************/
uint8_t LQ_MPU6050_GetId()
{
    uint8_t id = 0;
    LQ_MPU6050_Read_Reg(WHO_AM_I, &id); //获取陀螺仪设备 ID
    return id;
}
