
//#include "LQ_LSM6DSR_Soft.h"

#include "LQ_LSM6DSR_Hard.h"
#include "include.h"



// ---------------------------------硬件I2C方式读取LSM6DSR六轴陀螺仪---------------------------------


#ifdef HARDWARE_IIC // 硬件IIC方式读取LSM6DSR六轴陀螺仪
/**
 * @brief 向LSM6DSR传感器写入寄存器值（I2C接口）
 * @param reg 要写入的寄存器地址
 * @param dat 要写入的寄存器值
 * @return 1 表示操作成功
 * @note 此函数通过I2C接口与LSM6DSR传感器通信
 *       使用I2C_WriteNbyte函数实现单字节写入
 */
bit LSM6DSR_WriteReg(unsigned char reg, unsigned char dat)
{
    I2C_WriteNbyte(LSM6DSR_ADDR_7BIT, reg, &dat, 1);
    return 1;
}

/**
 * @brief  LSM6DSR 读多个寄存器
 * @param  reg  : 起始寄存器地址
 * @param  buf  : 数据缓冲区
 * @param  len  : 要读的字节数（≤255）
 * @return 1 成功
 */
unsigned char LSM6DSR_ReadReg(unsigned char reg, unsigned char *buf, unsigned char len)
{
    I2C_ReadNbyte(LSM6DSR_ADDR_7BIT, reg, buf, len);
    return 1;
}

unsigned char LQ_HARD_IIC_LSM60DSR_GetID(void)
{
    u8 id;
    LSM6DSR_ReadReg(LSM6DSR_WHO_AM, &id, 1); // WHO_AMI 寄存器
    return id;
}

void LQ_HARD_IIC_LSM60DSR_Init(void)
{
    I2C_InitTypeDef I2C_InitStr;

    I2C_InitStr.I2C_Speed = 10;             // 总线速度=Fosc/2/(Speed*2+4)   --> 40M/2/(10*2+4) = 400KHz
    I2C_InitStr.I2C_Enable = ENABLE;        // 使能I2C
    I2C_InitStr.I2C_Mode = I2C_Mode_Master; // 主模式
    I2C_InitStr.I2C_MS_WDTA = DISABLE;      // 主机模式无应答
    I2C_Init(&I2C_InitStr);
    NVIC_I2C_Init(I2C_Mode_Master, ENABLE, Priority_1);

    I2C_SW(I2C_P24_P23);                                           // I2C_SDA = P2_4; I2C_SCL = P2_3;
    GPIO_Init(GPIO_P2, GPIO_Pin_3 | GPIO_Pin_4, GPIO_Mode_Out_IN); // 准双向口

    // I2CCFG = 0xc2;  // 使能I2C主机模式
    // I2CPSCR = 0x00; // MSSPEED[13:6] //400,000Hz
    // I2CMSST = 0x00;

    // 加速度计52HZ（倾斜角检测功能工作在26HZ，因此加速度计ODR必须设置为>=26hz）,2g量程。
    LSM6DSR_WriteReg(LSM6DSR_CTRL1_XL, 0X20|0x10);       // 使能加速度计x,y,z轴
    LSM6DSR_WriteReg(LSM6DSR_CTRL9_XL, 0X38);       // 陀螺仪电平触发，加速度计高性能使能
    LSM6DSR_WriteReg(LSM6DSR_CTRL6_C, 0X40 | 0x10); // 陀螺仪高性能使能
    LSM6DSR_WriteReg(LSM6DSR_CTRL7_G, 0X80);        // 加速度计INT2引脚失能,陀螺仪数据INT2使能
    LSM6DSR_WriteReg(LSM6DSR_INT2_CTRL, 0X03);      // 陀螺仪12.5hz  2000dps
    LSM6DSR_WriteReg(LSM6DSR_CTRL2_G, 0X1C);        // 使能陀螺仪x,y,z轴
    LSM6DSR_WriteReg(LSM6DSR_CTRL10_C, 0X38);       // 陀螺仪高性能使能
}

void LQ_HARD_IIC_LSM60DSR_Read(signed short *ax, signed short *ay, signed short *az, signed short *gx, signed short *gy, signed short *gz)
{
    unsigned char buf[12];
    LSM6DSR_ReadReg(LSM6DSR_OUTX_L_GYRO, buf, 12);
    *gx = ((uint16)buf[1] << 8) | buf[0];
    *gy = ((uint16)buf[3] << 8) | buf[2];
    *gz = ((uint16)buf[5] << 8) | buf[4];
    *ax = ((uint16)buf[7] << 8) | buf[6];
    *ay = ((uint16)buf[9] << 8) | buf[8];
    *az = ((uint16)buf[11] << 8) | buf[10];
}


#elif defined HARDWARE_SPI // 硬件SPI方式读取LSM6DSR六轴陀螺仪

/*************************************************************************************************************************************
 *                                                       以下为SPI通信模式
 * ************************************************************************************************************************************/
// 初始化LSM60DSR传感器的硬件SPI接口
void LSM60DSR_HARD_Init(void)
{
    SPI_InitTypeDef SPI_InitStruct; // SPI初始化结构体

    // 配置SPI参数
    SPI_InitStruct.SPI_SSIG = ENABLE;          // 使用软件片选
    SPI_InitStruct.SPI_Enable = ENABLE;        // 使能SPI
    SPI_InitStruct.SPI_FirstBit = SPI_MSB;     // 高位先发送
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master; // 主机模式
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;   // 时钟极性高
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;  // 第二个边沿采样
    SPI_InitStruct.SPI_Speed = SPI_Speed_8;    // SPI时钟分频
    SPI_Init(&SPI_InitStruct);                 // 初始化SPI
    NVIC_SPI_Init(DISABLE, Priority_0);        // 配置SPI中断，禁用中断    ESPI = 0; // 使能SPI中断

    // 初始化P4.0引脚为推挽输出// 初始化P4.1-P4.3引脚为双向口
    GPIO_Init(GPIO_P4, GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3, GPIO_Mode_Out_IN);
    GPIO_Init(GPIO_P4, GPIO_Pin_0, GPIO_Mode_Out_PP);

    // 设置SPI引脚映射到P4.0-P4.3
    // SPI_S0 = 0; SPI_S1 = 1;
    // 00: P1.4 P1.5 P1.6 P1.7,
    // 01: P2.4 P2.5 P2.6 P2.7,
    // 10: P4.0 P4.1 P4.2 P4.3,
    // 11: P3.5 P3.4 P3.3 P3.2
    SPI_SW(SPI_P40_P41_P42_P43); // 第3组
    LSM6DSR_CK_STA(LOW);         // 初始时钟设置为低电平
    SPI_ClearFlag();             // (SPIF和WCOL)清除SPI传输完成标志,清除写冲突标志
    HSPllClkConfig(MCKSEL_HIRC, PLL_96M, 4); // 系统时钟选择,PLL时钟选择,时钟分频系数
    HSSPI_Enable();
}

/**
 * @brief 向LSM6DSR传感器写入寄存器值
 * @param reg 要写入的寄存器地址
 * @param val 要写入的寄存器值
 * @note 此函数通过SPI接口与LSM6DSR传感器通信
 *       1. 拉低片选信号
 *       2. 发送寄存器地址(bit7=0表示写操作)
 *       3. 发送要写入的数据
 *       4. 拉高片选信号
 */
void LSM6DSR_SPI_WriteReg(u8 reg, u8 val)
{
    LSM6DSR_CS_STA(LOW);  // 拉低片选信号,选中主线占用
    SPI_WriteByte(reg);   // 寄存器地址（bit7=0 表示写）
    SPI_WriteByte(val);   // 数据
    LSM6DSR_CS_STA(HIGH); // 拉高片选信号,解除主线占用
}

/**
 * @brief 从LSM6DSR传感器读取指定寄存器的值
 *
 * @param reg 要读取的寄存器地址
 * @return u8 读取到的寄存器值
 *
 * @note 该函数通过SPI接口与LSM6DSR传感器通信
 *       1. 拉低CS片选信号开始通信
 *       2. 发送寄存器地址(bit7=1表示读操作)
 *       3. 读取寄存器值(需虚写一字节时钟)
 *       4. 拉高CS片选信号结束通信
 */
u8 LSM6DSR_SPI_ReadReg(u8 reg)
{
    u8 val;
    LSM6DSR_CS_STA(LOW);
    SPI_WriteByte(reg | 0x80); // bit7=1 表示读
    val = SPI_ReadByte();      // 虚写一字节时钟，同时读回数据
    LSM6DSR_CS_STA(HIGH);
    return val;
}

/*************************************************************************
 * @code    void LSM6DSR_SPI_ReadNByte(u8 reg, u16 buf_len)
 * @brief   读取 LSM6DSR 的六轴原始数据
 * @param   u8 _Mode  ,设置量程
 * @return  none.
 * @date    2025-12-28.
 * @example void LSM6D_SetAcc_FullScale(u8 _Mode)
 *************************************************************************/

void LSM6DSR_SPI_ReadNByte(u8 reg, u8* buffer,u8 sizel)
{
    LSM6DSR_CS_STA(LOW);
    SPI_Read_NByte(reg, buffer, sizel);
    LSM6DSR_CS_STA(HIGH);
}


/*************************************************************************
 * @code    void LSM6DSR_Read_Data(int16 *ax, int16 *ay, int16 *az, int16 *gx, int16 *gy, int16 *gz)
 * @brief   读取 LSM6DSR 的六轴原始数据
 * @param   ax : 存放 x 轴加速度数据的指针.
 * @param   ay : 存放 y 轴加速度数据的指针.
 * @param   az : 存放 z 轴加速度数据的指针.
 * @param   gx : 存放 x 轴陀螺仪数据的指针.
 * @param   gy : 存放 y 轴陀螺仪数据的指针.
 * @param   gz : 存放 z 轴陀螺仪数据的指针.
 * @return  none.
 * @date    2025-12-28.
 * @example LSM6DSR_Read_Data(&ax, &ay, &az, &gx, &gy, &gz);
 *************************************************************************/
void LSM6DSR_Read_Data(int16 *ax, int16 *ay, int16 *az, int16 *gx, int16 *gy, int16 *gz)
{
    u8 buf[12] = {0};
    
    LSM6DSR_SPI_ReadNByte(HLSM6DSR_OUTX_L_GYRO, buf,12);
    *gx = (u16)((buf[1] << 8) | buf[0]);
    *gy = (u16)((buf[3] << 8) | buf[2]);
    *gz = (u16)((buf[5] << 8) | buf[4]);
    *ax = (u16)((buf[7] << 8) | buf[6]);
    *ay = (u16)((buf[9] << 8) | buf[8]);
    *az = (u16)((buf[11] << 8) | buf[10]);
}

/*************************************************************************
 * @code    void LSM6DSR_SetAcc_FullScale(u8 _Mode)
 * @brief   读取 LSM6DSR 的六轴原始数据
 * @param   u8 _Mode  ,设置量程
 * @return  none.
 * @date    2025-12-28.
 * @example void LSM6D_SetAcc_FullScale(u8 _Mode)
 *************************************************************************/
void LSM6DSR_SetAcc_FullScale(u8 _Mode)
{
	u8 buf = 0;
	buf = LSM6DSR_SPI_ReadReg(HLSM6DSR_CTRL1_XL);	 // 读取CTRL1_XL寄存器配置
	buf |= _Mode;			                         // 设置加速度计的满量程
	LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL1_XL, buf);
}

/*************************************************************************
 * @code    void LSM6DSR_SetGyro_Rate(u8 _Mode)
 * @brief   读取 LSM6DSR 的六轴原始数据
 * @param   u8 _Mode  ,设置量程
 * @return  none.
 * @date    2025-12-28.
 * @example void LSM6D_SetAcc_FullScale(u8 _Mode)
 *************************************************************************/
void LSM6DSR_SetGyro_Rate(u8 _Mode)
{
	u8 buf = 0;
	buf = LSM6DSR_SPI_ReadReg(HLSM6DSR_CTRL2_G);	 // 读取CTRL1_XL寄存器配置
	buf |= _Mode;			                         // 设置加速度计的满量程
	LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL2_G, buf);
}


void LQ_HARD_SPI_LSM60DSR_Init(void)
{
    LSM60DSR_HARD_Init();

    LSM6DSR_CS_STA(LOW);
    
    /* 加速度计控制寄存器1 (r/w) bit1 : 0 -> 一级数字滤波输出. 1 -> LPF2第二级滤波输出
     LSM6DSR_CTRL1_XL : 加速度计量程选择, 默认为 00 -> ±2g, 01 -> ±16g, 10 -> ±4g, 11 -> ±8g
    */
    LSM6DSR_SPI_WriteReg(HLSM6DSR_FUNC_CFG, 0x00);        // 选择LSM6DSR主寄存器，而非FIFO拓展
    
    LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL1_XL, RATE_833Hz);  // 设置，加速度计 回报率
    LSM6DSR_SetAcc_FullScale(ACC_FS_XL_2G);
    LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL9_XL, 0x38);        // 使能加速度计x,y,z轴
    LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL6_C, 0x40 | 0x10);  // 陀螺仪电平触发，加速度计高性能使能
    LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL7_G, 0x80);         // 陀螺仪高性能使能
    LSM6DSR_SPI_WriteReg(HLSM6DSR_INT2_CTRL, 0x03);       // 加速度计INT2引脚失能,陀螺仪数据INT2使能
    LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL2_G, 0x0C);         // 陀螺仪2000dps（0x0C）,选择833Hz（0x70) 默认1c, 12.5K
    LSM6DSR_SetGyro_Rate(RATE_833Hz);                     // 设置，加速度计 回报率,注意陀螺仪和加速度计 回报率必须一致
    LSM6DSR_SPI_WriteReg(HLSM6DSR_CTRL10_C, 0x38);        // 使能陀螺仪x,y,z轴
    delay_ms(5);

    LSM6DSR_SPI_WriteReg(HLSM6DSR_FUNC_CFG, 0x00);        // 选择LSM6DSR主寄存器，而非FIFO拓展
}

#endif                     // HARDWARE_SPI
