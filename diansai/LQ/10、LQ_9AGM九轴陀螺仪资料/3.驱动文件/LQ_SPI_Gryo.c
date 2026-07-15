
#include "include.h"

void Test_SPI_Gyro(void)
{
    char txt[30];
    short aacx, aacy, aacz;    // 加速度传感器原始数据
    short gyrox, gyroy, gyroz; // 陀螺仪原始数据
    short magx,magy,magz;           //9轴陀螺仪的磁力计原始数据
    unsigned char res;
    GPIO_LED_Init();        //LED初始化
    Display_Init();     //屏幕初始化
    res = SPI_Gryo_Init();
    Display_CLS(U16_BLACK);
    sprintf(txt,"returnID:%2x",res);
    Display_showString(20, 12, txt, U16_WHITE, U16_BLACK, 16);


    Display_showString(15, 0, "   LQ LQ9AGMV Test", U16_WHITE, U16_BLACK, 24);


    while (1)
    {
        ICM_Get_Raw_data(&aacx, &aacy, &aacz, &gyrox, &gyroy, &gyroz); // 得到加速度传感器数据
        sprintf((char *)txt, "ax:%06d", aacx);
        Display_showString(0, 2, txt,U16_WHITE, U16_BLACK, 16);

        sprintf((char *)txt, "ay:%06d", aacy);
        Display_showString(0, 3, txt, U16_WHITE, U16_BLACK, 16);
        sprintf((char *)txt, "az:%06d", aacz);
        Display_showString(0, 4, txt, U16_WHITE, U16_BLACK, 16);

        sprintf((char *)txt, "gx:%06d", gyrox);
        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16);

        sprintf((char *)txt, "gy:%06d", gyroy);
        Display_showString(0, 6, txt, U16_WHITE, U16_BLACK, 16);

        sprintf((char *)txt, "gz:%06d", gyroz);
        Display_showString(0, 7, txt, U16_WHITE, U16_BLACK, 16);


        Gyro_Get_Mag_data(&magx,&magy,&magz);
        sprintf((char*)txt,"mx:%06d",magx);
        Display_showString(0, 8, txt, U16_WHITE, U16_BLACK, 16);
        sprintf((char*)txt,"my:%06d",magy);
        Display_showString(0, 9, txt, U16_WHITE, U16_BLACK, 16);
        sprintf((char*)txt,"mz:%06d",magz);
        Display_showString(0, 10, txt, U16_WHITE, U16_BLACK, 16);


        Delay_Ms(5);
    }
}
/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void delayms_icm(uint16 ms)
@功能说明：不精确延时
@参数说明：需要延时时间
@函数返回：无
@调用方法：delayms_icm(100);
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void delayms_icm(uint16 ms)
{
    volatile unsigned long i = 0;
    while (ms--)
    {
        for (i = 0; i < 30000; ++i)
        {
            __asm("NOP"); /* delay */
        }
    }
}

//9轴陀螺仪地磁计的寄存器写数据
uint8_t SPI_Mag_Write_Reg(uint8_t addr, uint8_t data)
{
    uint32_t count = 0;
    ICM_Write_Byte(0x17, 0x00);
    ICM_Write_Byte(0x15, 0x38);
    ICM_Write_Byte(0x16, addr);
    ICM_Write_Byte(0x21, data);
    ICM_Write_Byte(0x14, 0x4c);
    while((ICM_Read_Byte(0x22) & 0x80) == 0)
    {
        if(1000 < count++){
            return 1;
        }
        Delay_Us(100);
    }
    return 0;
}

//向9轴陀螺仪地磁计的寄存器读数据
uint8_t SPI_Mag_Read_Reg(uint8_t addr)
{
    uint16_t count = 0;
    ICM_Write_Byte(0x17, 0x01);
    ICM_Write_Byte(0x15, 0x39);
    ICM_Write_Byte(0x16, addr);
    ICM_Write_Byte(0x14, 0x4c);
    while((ICM_Read_Byte(0x22) & 0x01) == 0)
    {
        if(1000 < count++){
            break;
        }
        Delay_Us(100);
    }
    return ICM_Read_Byte(0x02);
}
//6轴陀螺仪LSM6DSR和九轴陀螺仪LQ9AGMV的检测
uint8_t Gyro_LSM6DSR_LQ9AGMV_Check(void)
{
    uint8_t res;
    ICM_Write_Byte(LSM6DSR_FUNC_CFG,0x00);  //主操作
    ICM_Write_Byte(LSM6DSR_CTRL3_C, 0x05);
    Delay_Ms(2);
    ICM_Write_Byte(LSM6DSR_FUNC_CFG,0x00);
    res = ICM_Read_Byte(WHO_AM_LSM6D);
    if(res == DRV_ID_LSM6D){
        for(int i = 0;i <= 3; i++)
        {
            ICM_Write_Byte(0x14, 0x80);
            Delay_Ms(5);
            ICM_Write_Byte(0x14, 0x00);
            Delay_Ms(5);
            ICM_Write_Byte(LSM6DSR_CTRL1_XL,0X20);
            ICM_Write_Byte(LSM6DSR_FUNC_CFG, 0x40);
            if(SPI_Mag_Read_Reg(0x0f) == Mag_ID){
                res = Mag_ID;
                break;
            }
        }
        ICM_Write_Byte(LSM6DSR_FUNC_CFG, 0x00);
    }

    return res;
}
//读取9轴陀螺仪LQ9AGMV的3轴磁力计
void Gyro_Get_Mag_data(short *magx,short *magy,short *magz)
{
    unsigned char buf[6];

    ICM_Write_Byte(LSM6DSR_FUNC_CFG, 0x40);
    ICM_Read_Len(0x02,6,buf);
    *magx=(short)(((uint16_t)buf[1]<<8)|buf[0]);
    *magy=(short)(((uint16_t)buf[3]<<8)|buf[2]);
    *magz=(short)(((uint16_t)buf[5]<<8)|buf[4]);
    ICM_Write_Byte(LSM6DSR_FUNC_CFG, 0x00);
}
/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void Test_ICM20689(void)
@功能说明：初始化 ICM20689
@参数说明：无
@函数返回：0：初始化成功   1：失败
@调用方法：ICM20689_Init();
@备    注：初始化时调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char SPI_Gryo_Init(void)
{

    unsigned char res,add;
    QSPI_InitConfig(QSPI2_CLK_P15_3, QSPI2_MISO_P15_7, QSPI2_MOSI_P15_6, QSPI2_CS_P15_2, 4500000, 1);
    res = Gyro_LSM6DSR_LQ9AGMV_Check();
    add = res;

    ICM_Write_Byte(LSM6DSR_CTRL1_XL,0X20);      //加速度计52HZ（倾斜角检测功能工作在26HZ，因此加速度计ODR必须设置为>=26hz）,2g量程。
    ICM_Write_Byte(LSM6DSR_CTRL9_XL,0X38);      //使能加速度计x,y,z轴
    ICM_Write_Byte(LSM6DSR_CTRL6_C,0X40|0x10);  //陀螺仪电平触发，加速度计高性能使能
    ICM_Write_Byte(LSM6DSR_CTRL7_G,0X80);       //陀螺仪高性能使能
    ICM_Write_Byte(LSM6DSR_INT2_CTRL,0X03);     //加速度计INT2引脚失能,陀螺仪数据INT2使能
    ICM_Write_Byte(LSM6DSR_CTRL2_G,0X1C);       //陀螺仪12.5hz  2000dps
    ICM_Write_Byte(LSM6DSR_CTRL10_C,0X38);      //使能陀螺仪x,y,z轴

    ICM_Write_Byte(LSM6DSR_FUNC_CFG, 0x40);
    ICM_Write_Byte(0x14, 0x80);
    Delay_Ms(1);
    ICM_Write_Byte(0x14, 0x00);
    Delay_Ms(1);
    SPI_Mag_Write_Reg(0x21,0x04);   //陀螺仪复位
    Delay_Ms(5);
    SPI_Mag_Write_Reg(0x21,0x00);
    Delay_Ms(5);
    SPI_Mag_Write_Reg(0x20,0x7f);
    SPI_Mag_Write_Reg(0x23,0x0c);
    SPI_Mag_Write_Reg(0x22,0x00);
    SPI_Mag_Write_Reg(0x24,0x00);
    SPI_Mag_Write_Reg(0x30,0x02);
    ICM_Write_Byte(0x17, 0x06);
    ICM_Write_Byte(0x15, 0x39);
    ICM_Write_Byte(0x16, 0x28);
    ICM_Write_Byte(0x14, 0x4c);
    while((ICM_Read_Byte(0x22) & 0x01) == 0);
    ICM_Write_Byte(0x14, 0x6c);
    ICM_Write_Byte(LSM6DSR_FUNC_CFG, 0x00);

    return add;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Set_Gyro_Fsr(u8 fsr)
@功能说明：设置陀螺仪传感器满量程范围
@参数说明：Fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
@函数返回：
@调用方法：
@备    注：内部调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Set_Gyro_Fsr(unsigned char fsr)
{
    return ICM_Write_Byte(ICM_GYRO_CFG_REG, fsr << 3);
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Set_Accel_Fsr(u8 fsr)
@功能说明：设置LQ20689陀螺仪传感器满量程范围
@参数说明：fsr:0,±2g;1,±4g;2,±8g;3,±16g
@函数返回：
@调用方法：
@备    注：内部调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Set_Accel_Fsr(unsigned char fsr)
{
    return ICM_Write_Byte(ICM_ACCEL_CFG_REG, fsr << 3);
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Set_LPF(u16 lpf)
@功能说明：设置LQ20689数字低通滤波器
@参数说明：lpf:数字低通滤波频率(Hz)
@函数返回：
@调用方法：
@备    注：内部调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Set_LPF(uint16 lpf)
{
    unsigned char data = 0;
    if (lpf >= 188)
        data = 1;
    else if (lpf >= 98)
        data = 2;
    else if (lpf >= 42)
        data = 3;
    else if (lpf >= 20)
        data = 4;
    else if (lpf >= 10)
        data = 5;
    else
        data = 6;
    return ICM_Write_Byte(ICM_CFG_REG, data); // 设置数字低通滤波器
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Set_Rate(u16 rate)
@功能说明：设置LQ20689陀螺仪传感器满量程范围
@参数说明：rate:4~1000(Hz)
@函数返回：
@调用方法：
@备    注：内部调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Set_Rate(uint16 rate)
{
    unsigned char data;
    if (rate > 1000)
        rate = 1000;
    if (rate < 4)
        rate = 4;
    data = 1000 / rate - 1;
    ICM_Write_Byte(ICM_SAMPLE_RATE_REG, data); // 设置数字低通滤波器
    return ICM_Set_LPF(rate / 2);              // 自动设置LPF为采样率的一半
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：u16 ICM_Get_Temperature(void)
@功能说明：读取温度数据
@参数说明：无
@函数返回：温度值(扩大了100倍)
@调用方法：ICM_Get_Temperature();
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
short ICM_Get_Temperature(void)
{
    unsigned char buf[3];
    short raw;
    float temp;
    ICM_Read_Len(ICM_TEMP_OUTH_REG, 2, buf);
    raw = ((uint16)buf[1] << 8) | buf[2];
    temp = 21 + ((double)raw) / 333.87;
    return (short)temp * 100;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Get_Gyroscope(u16 *gx,u16 *gy,u16 *gz)
@功能说明：读取加速度融合数据
@参数说明：加速度三轴数据
@函数返回：无
@调用方法：
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Get_Gyroscope(short *gx, short *gy, short *gz)
{
    unsigned char buf[7], res;
    res = ICM_Read_Len(ICM_GYRO_XOUTH_REG, 6, buf);
    if (res == 0)
    {
        *gx = ((uint16)buf[1] << 8) | buf[2];
        *gy = ((uint16)buf[3] << 8) | buf[4];
        *gz = ((uint16)buf[5] << 8) | buf[6];
    }
    return res;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Get_Accelerometer(u16 *ax,u16 *ay,u16 *az)
@功能说明：读取角速度融合数据
@参数说明：角速度三轴数据
@函数返回：无
@调用方法：
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Get_Accelerometer(short *ax, short *ay, short *az)
{
    unsigned char buf[7], res;
    res = ICM_Read_Len(ICM_ACCEL_XOUTH_REG, 6, buf);
    if (res == 0)
    {
        *ax = ((uint16)buf[1] << 8) | buf[2];
        *ay = ((uint16)buf[3] << 8) | buf[4];
        *az = ((uint16)buf[5] << 8) | buf[6];
    }
    return res;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Get_Raw_data(u16 *ax,u16 *ay,u16 *az,u16 *gx,u16 *gy,u16 *gz)
@功能说明：读取陀螺仪融合数据
@参数说明：角速度与加速度的原始数据
@函数返回：无
@调用方法：ICM_Get_Raw_data(&aacx,&aacy,&aacz,&gyrox,&gyroy,&gyroz);
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Get_Raw_data(short *ax, short *ay, short *az, short *gx, short *gy, short *gz)
{
    unsigned char buf[15], res;

    res = ICM_Read_Len(LSM6DSR_OUTX_L_GYRO,12,buf);
    *gx=(uint16)(((uint16_t)buf[1]<<8)|buf[0]);
    *gy=(uint16)(((uint16_t)buf[3]<<8)|buf[2]);
    *gz=(uint16)(((uint16_t)buf[5]<<8)|buf[4]);
    *ax=(uint16)(((uint16_t)buf[7]<<8)|buf[6]);
    *ay=(uint16)(((uint16_t)buf[9]<<8)|buf[8]);
    *az=(short)(((uint16_t)buf[11]<<8)|buf[10]);


    return res;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Read_Len(u8 reg,u8 len,u8 *buf)
@功能说明：u8 reg起始寄存器,u8 *buf数据指针,u16 len长度
@参数说明：无
@函数返回：无
@调用方法：ICM_Read_Len(ICM_GYRO_XOUTH_REG,6,buf);
@备    注：內部調用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Read_Len(unsigned char reg, unsigned char len, unsigned char *buf)
{
    buf[0] = reg | 0x80;
    /* 写入要读的寄存器地址 */
    return QSPI_ReadWriteNByte(QSPI2, buf, buf, len + 1);
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void ICM_Write_Byte(u8 reg,u8 value)
@功能说明：向寄存器写数据
@参数说明：reg（寄存器）,value（数据）
@函数返回：无
@调用方法：
@备    注：内部调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Write_Byte(unsigned char reg, unsigned char value)
{
    unsigned char buff[2];

    buff[0] = reg;                                    // 先发送寄存器
    buff[1] = value;                                  // 再发送数据
    return QSPI_ReadWriteNByte(QSPI2, buff, buff, 2); // 发送buff里数据，并采集到 buff里
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：u8 ICM_Read_Byte(u8 reg)
@功能说明：向寄存器读数据
@参数说明：reg（寄存器）,
@函数返回：value（数据）
@调用方法：
@备    注：内部调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
unsigned char ICM_Read_Byte(unsigned char reg)
{
    unsigned char buff[2];
    buff[0] = reg | 0x80; // 先发送寄存器
//    GYRO_CS_L;
    QSPI_ReadWriteNByte(QSPI2, buff, buff, 2);
//    GYRO_CS_H;
    return buff[1];
}
