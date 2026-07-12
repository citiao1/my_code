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

#include "include.h"
#include "LQ_LSM6DSR_Soft.h"


#if defined(LSM6DSR_USE_SOFT_I2C)
static Soft_I2C_cfg_t lsm6dsr_i2c_cfg;
#elif defined(LSM6DSR_USE_SOFT_SPI)
static Soft_SPI_cfg_t lsm6dsr_spi_cfg;
#endif

//=============================================================================================================
//                                                 内部调用函数
//=============================================================================================================

/*************************************************************************
 * @code    void lsm6dsr_write_reg(u8 reg, u8 buf)
 * @brief   写 LSM6DSR 的 1 个字节数据到指定寄存器
 * @param   reg : 寄存器地址.
 * @param   buf : 要写入的数据.
 * @return  none.
 * @date    2025-12-28.
 * @example lsm6dsr_write_reg(0x68, 0x10, 0x0F);
 *************************************************************************/
void lsm6dsr_write_reg(u8 reg, u8 buf)
{
#if defined(LSM6DSR_USE_SOFT_I2C)
    soft_i2c_write_n_byte(&lsm6dsr_i2c_cfg, reg, &buf, 1);
#elif defined(LSM6DSR_USE_SOFT_SPI)
    soft_spi_write_byte(&lsm6dsr_spi_cfg, reg, buf);
#endif
}

/*************************************************************************
 * @code    u8 lsm6dsr_read_reg(u8 reg)
 * @brief   从 LSM6DSR 的指定寄存器读 1 个字节数据到指定缓冲区
 * @param   reg : 寄存器地址.
 * @return  读取到的数据.
 * @date    2025-12-28.
 * @example u8 val = lsm6dsr_read_reg(0x68, 0x10);
 *************************************************************************/
u8 lsm6dsr_read_reg(u8 reg)
{
    u8 dat = 0;
#if defined(LSM6DSR_USE_SOFT_I2C)
    soft_i2c_read_n_byte(&lsm6dsr_i2c_cfg, reg, &dat, 1);
#elif defined(LSM6DSR_USE_SOFT_SPI)
    dat = soft_spi_read_byte(&lsm6dsr_spi_cfg, reg);
#endif
    return dat;
}

/*************************************************************************
 * @code    void lsm6dsr_write_n_reg(u8 reg, u8 *buf, u8 len)
 * @brief   写 LSM6DSR 的 n 个字节数据到指定寄存器
 * @param   reg : 寄存器地址.
 * @param   buf : 要写入的数据.
 * @param   len : 要写入的数据长度.
 * @return  none.
 * @date    2025-12-28.
 * @example lsm6dsr_write_n_reg(0x68, 0x10, buf, 2);
 *************************************************************************/
void lsm6dsr_write_n_reg(u8 reg, u8 *buf, u8 len)
{
#if defined(LSM6DSR_USE_SOFT_I2C)
    soft_i2c_write_n_byte(&lsm6dsr_i2c_cfg, reg, buf, len);
#elif defined(LSM6DSR_USE_SOFT_SPI)
    soft_spi_write_n_byte(&lsm6dsr_spi_cfg, reg, buf, len);
#endif
}

/*************************************************************************
 * @code    void lsm6dsr_read_n_reg(u8 reg, u8 *buf, u8 len)
 * @brief   从 LSM6DSR 的指定寄存器读 1 个字节数据到指定缓冲区
 * @param   reg : 寄存器地址.
 * @param   buf : 读取的数据存放缓冲区.
 * @param   len : 读取到的数据长度.
 * @return  none.
 * @date    2025-12-28.
 * @example lsm6dsr_read_n_reg(0x68, 0x10, buf, 2);
 *************************************************************************/
void lsm6dsr_read_n_reg(u8 reg, u8 *buf, u8 len)
{
#if defined(LSM6DSR_USE_SOFT_I2C)
    soft_i2c_read_n_byte(&lsm6dsr_i2c_cfg, reg, buf, len);
#elif defined(LSM6DSR_USE_SOFT_SPI)
    soft_spi_read_n_byte(&lsm6dsr_spi_cfg, reg, buf, len);
#endif
}



//=============================================================================================================
//                                                 外部调用函数
//=============================================================================================================
/*************************************************************************
 * @code    u8 lsm6dsr_read_id(void)
 * @brief   读取 LSM6DSR 的设备 ID
 * @param   none.
 * @return  读取到的设备 ID.
 * @date    2025-12-28.
 * @example lsm6dsr_init();
 *************************************************************************/
u8 lsm6dsr_read_id(void)
{
    return lsm6dsr_read_reg(LSM6DSR_WHO_AM);
}

/*************************************************************************
 * @code    void lsm6dsr_init(void)
 * @brief   初始化 LSM6DSR 六轴传感器
 * @param   none.
 * @return  none.
 * @date    2025-12-28.
 * @example lsm6dsr_init();
 *************************************************************************/
void lsm6dsr_init(void)
{
#if defined(LSM6DSR_USE_SOFT_I2C)
    gpio_init_pin(LSM6DSR_AD0_PIN, GPIO_Mode_Out_PP);    // 设置AD0引脚为推挽输出
    gpio_write_pin(LSM6DSR_AD0_PIN, LOW);                // 设置AD0引脚为低电平,则LSM6DSR的I2C地址为0x6A
    lsm6dsr_i2c_cfg.scl  = LSM6DSR_I2C_SCL;              // 设置I2C的SCL引脚
    lsm6dsr_i2c_cfg.sda  = LSM6DSR_I2C_SDA;              // 设置I2C的SDA引脚
    lsm6dsr_i2c_cfg.addr = LSM6DSR_ADDR;                 // 设置I2C的地址
    soft_i2c_init(&lsm6dsr_i2c_cfg);
#elif defined(LSM6DSR_USE_SOFT_SPI)
    lsm6dsr_spi_cfg.sck  = LSM6DSR_SPI_SCK;
    lsm6dsr_spi_cfg.miso = LSM6DSR_SPI_MISO;
    lsm6dsr_spi_cfg.mosi = LSM6DSR_SPI_MOSI;
    lsm6dsr_spi_cfg.cs   = LSM6DSR_SPI_CS;
    lsm6dsr_spi_cfg.mode = Soft_SPI_Mode_3;
    soft_spi_init(&lsm6dsr_spi_cfg);

#endif
    delay_ms(50);      // 延时100ms

    lsm6dsr_write_reg(LSM6DSR_CTRL1_XL , 0x21);         // 加速度计52Hz(倾斜角检测功能在26Hz,因此加速度计必须设置为>=26Hz), 2g量程
    lsm6dsr_write_reg(LSM6DSR_CTRL9_XL , 0x38);         // 使能加速度计x, y, z轴
    lsm6dsr_write_reg(LSM6DSR_CTRL6_C  , 0X40|0x10);    // 陀螺仪电平触发，加速度计高性能使能
    lsm6dsr_write_reg(LSM6DSR_CTRL7_G  , 0X80);         // 陀螺仪高性能使能
    lsm6dsr_write_reg(LSM6DSR_INT2_CTRL, 0X03);         // 加速度计INT2引脚失能,陀螺仪数据INT2使能
    lsm6dsr_write_reg(LSM6DSR_CTRL2_G  , 0X1C);         // 陀螺仪12.5hz  2000dps
    lsm6dsr_write_reg(LSM6DSR_CTRL10_C , 0X38);         // 使能陀螺仪x,y,z轴
    delay_ms(10);
    lsm6dsr_write_reg(LSM6DSR_FUNC_CFG, 0x00);
}

/*************************************************************************
 * @code    void lsm6dsr_read_raw_data(int16 *ax, int16 *ay, int16 *az, int16 *gx, int16 *gy, int16 *gz)
 * @brief   读取 LSM6DSR 的六轴原始数据
 * @param   ax : 存放 x 轴加速度数据的指针.
 * @param   ay : 存放 y 轴加速度数据的指针.
 * @param   az : 存放 z 轴加速度数据的指针.
 * @param   gx : 存放 x 轴陀螺仪数据的指针.
 * @param   gy : 存放 y 轴陀螺仪数据的指针.
 * @param   gz : 存放 z 轴陀螺仪数据的指针.
 * @return  none.
 * @date    2025-12-28.
 * @example lsm6dsr_read_raw_data(&ax, &ay, &az, &gx, &gy, &gz);
 *************************************************************************/
void lsm6dsr_read_raw_data(int16 *ax, int16 *ay, int16 *az, int16 *gx, int16 *gy, int16 *gz)
{
    u8 buf[12] = {0};
    lsm6dsr_read_n_reg(LSM6DSR_OUTX_L_GYRO, buf, 12);
    *gx = ((u16)buf[1]  << 8) | buf[0];
    *gy = ((u16)buf[3]  << 8) | buf[2];
    *gz = ((u16)buf[5]  << 8) | buf[4];
    *ax = ((u16)buf[7]  << 8) | buf[6];
    *ay = ((u16)buf[9]  << 8) | buf[8];
    *az = ((u16)buf[11] << 8) | buf[10];
}



