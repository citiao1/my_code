#include "include.h"

/*************************************************************************
 * @code    void i2c_delay()
 * @brief   模拟I2C延时.
 * @param   us : 延时时间，单位：微秒.
 * @return  none.
 * @date    2025-12-25.
 * @note    模拟I2C内部使用.
 *************************************************************************/
static void i2c_delay(void)
{
    delay_us(2); // 延时2毫秒
}

/*************************************************************************
 * @code    void soft_i2c_init(Soft_I2C_cfg_t *cfg)
 * @brief   模拟I2C初始化.
 * @param   cfg : 软件I2C配置参数.
 * @return  none.
 * @date    2025-12-25.
 *************************************************************************/
void soft_i2c_init(Soft_I2C_cfg_t *cfg)
{
    // 初始化SCL和SDA引脚
    Soft_I2C_SCL_Out(cfg->scl);     // 配置 SCL 引脚为输出模式
    Soft_I2C_SDA_Out(cfg->sda);     // 配置 SDA 引脚为输出模式
    gpio_pull_up(cfg->scl, ENABLE); // 上拉
    gpio_pull_up(cfg->sda, ENABLE); // 上拉
}

/*************************************************************************
 * @code    void soft_i2c_start(Soft_I2C_cfg_t *cfg)
 * @brief   模拟I2C起始信号.
 * @param   cfg : 软件I2C配置参数.
 * @return  none.
 * @date    2025-12-25.
 * @note    模拟I2C内部使用.
 *************************************************************************/
void soft_i2c_start(Soft_I2C_cfg_t *cfg)
{
    Soft_I2C_SDA_Out(cfg->sda);
    Soft_I2C_SDA_H(cfg->sda);
    Soft_I2C_SCL_H(cfg->scl);
    i2c_delay();
    i2c_delay();
    Soft_I2C_SDA_L(cfg->sda);   // START:when CLK is high,DATA change form high to low
    i2c_delay();
    Soft_I2C_SCL_L(cfg->scl);   // 钳住I2C总线，准备发送或接收数据
}

/*************************************************************************
 * @code    void soft_i2c_stop(Soft_I2C_cfg_t *cfg)
 * @brief   模拟I2C停止信号.
 * @param   cfg : 软件I2C配置参数.
 * @return  none.
 * @date    2025-12-25.
 * @note    模拟I2C内部使用.
 *************************************************************************/
void soft_i2c_stop(Soft_I2C_cfg_t *cfg)
{
    Soft_I2C_SDA_Out(cfg->sda);
    i2c_delay();
    Soft_I2C_SCL_L(cfg->scl);
    Soft_I2C_SDA_L(cfg->sda);   // STOP:when CLK is high DATA change form low to high
    i2c_delay();
    Soft_I2C_SCL_H(cfg->scl);
    i2c_delay();
    Soft_I2C_SDA_H(cfg->sda);   // 发送I2C总线结束信号
    i2c_delay();
}

/*************************************************************************
 * @code    void soft_i2c_wait_ack(Soft_I2C_cfg_t *cfg)
 * @brief   模拟I2C等待应答信号.
 * @param   cfg : 软件I2C配置参数.
 * @return  none.
 * @date    2025-12-25.
 * @note    模拟I2C内部使用 有效应答：从机第9个 SCL=0 时 SDA 被从机拉低,并且 SCL = 1时 SDA依然为低.
 *************************************************************************/
u8 soft_i2c_wait_ack(Soft_I2C_cfg_t *cfg)
{
    u8 ucErrTime = 0;
    Soft_I2C_SDA_In(cfg->sda);
    Soft_I2C_SDA_H(cfg->sda);
    i2c_delay();
    Soft_I2C_SCL_H(cfg->scl);
    i2c_delay();
    while (Soft_I2C_SDA_READ(cfg->sda))
    {
        ucErrTime++;
        if (ucErrTime > 100)
        {
            soft_i2c_stop(cfg);
            return 1;
        }
    }
    Soft_I2C_SCL_L(cfg->scl); // 时钟输出0
    return 0;
}

/*************************************************************************
 * @code    void soft_i2c_ack(Soft_I2C_cfg_t *cfg)
 * @brief   模拟I2C产生ACK应答.
 * @param   cfg : 软件I2C配置参数.
 * @return  none.
 * @date    2025-12-25.
 * @note    内部调用 主机接收完一个字节数据后，主机产生的ACK通知从机一个字节数据已正确接收.
 *************************************************************************/
void soft_i2c_ack(Soft_I2C_cfg_t *cfg)
{
    Soft_I2C_SCL_L(cfg->scl);
    Soft_I2C_SDA_Out(cfg->sda);
    Soft_I2C_SDA_L(cfg->sda);
    i2c_delay();
    Soft_I2C_SCL_H(cfg->scl);
    i2c_delay();
    Soft_I2C_SCL_L(cfg->scl);
}

/*************************************************************************
 * @code    void soft_i2c_nack(Soft_I2C_cfg_t *cfg)
 * @brief   模拟I2C不产生ACK应答.
 * @param   cfg : 软件I2C配置参数.
 * @return  none.
 * @date    2025-12-25.
 * @note    内部调用 主机接收完最后一个字节数据后，主机产生的NACK通知从机发送结束，释放SDA,以便主机产生停止信号.
 *************************************************************************/
void soft_i2c_nack(Soft_I2C_cfg_t *cfg)
{
    Soft_I2C_SCL_L(cfg->scl);
    Soft_I2C_SDA_Out(cfg->sda);
    Soft_I2C_SDA_H(cfg->sda);
    i2c_delay();
    Soft_I2C_SCL_H(cfg->scl);
    i2c_delay();
    Soft_I2C_SCL_L(cfg->scl);
}

/*************************************************************************
 * @code    void soft_i2c_send_byte(Soft_I2C_cfg_t *cfg, u8 data_t)
 * @brief   模拟I2C发送一个字节.
 * @param   cfg   : 软件I2C配置参数.
 * @param   ndata : 发送的字节.
 * @return  none.
 * @date    2025-12-25.
 * @example soft_i2c_send_byte(&cfg, 0x12); // 模拟I2C发送一个字节.
 *************************************************************************/
void soft_i2c_send_byte(Soft_I2C_cfg_t *cfg, u8 data_t)
{
    u8 t;
    Soft_I2C_SDA_Out(cfg->sda);
    Soft_I2C_SCL_L(cfg->scl);   // 拉低时钟开始数据传输
    for (t = 0; t < 8; t++)
    {
        if (data_t & 0x80) {
            Soft_I2C_SDA_H(cfg->sda);
        } else {
            Soft_I2C_SDA_L(cfg->sda);
        }
        Soft_I2C_SCL_H(cfg->scl);
        i2c_delay();
        data_t <<= 1;
        i2c_delay();
        Soft_I2C_SCL_L(cfg->scl);
        i2c_delay();
    }
    i2c_delay();
}

/*************************************************************************
 * @code    u8 soft_i2c_read_byte_internal(Soft_I2C_cfg_t *cfg, u8 ack)
 * @brief   模拟I2C读取一个字节(内部使用).
 * @param   cfg : 软件I2C配置参数.
 * @param   ack : 为 1 时，主机数据还没接收完，为0 时主机数据已全部接收完成.
 * @return  接收到的字节.
 * @date    2025-12-25.
 *************************************************************************/
u8 soft_i2c_read_byte_internal(Soft_I2C_cfg_t *cfg, u8 ack)
{
    u8 i, receive = 0;
    Soft_I2C_SDA_In(cfg->sda);
    for (i = 0; i < 8; i++)
    {
        Soft_I2C_SCL_L(cfg->scl);
        i2c_delay();
        Soft_I2C_SCL_H(cfg->scl);
        receive <<= 1;
        if (Soft_I2C_SDA_READ(cfg->sda))
            receive++; // 从机发送的电平
        i2c_delay();
    }
    if (ack)
        soft_i2c_ack(cfg);     // 发送ACK
    else
        soft_i2c_nack(cfg);    // 发送nACK
    return receive;
}

/*************************************************************************
 * @code    u8 soft_i2c_read_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *buf)
 * @brief   模拟I2C读取指定设备 指定寄存器的一个值.
 * @param   cfg      : 软件I2C配置参数.
 * @param   reg      : 目标寄存器.
 * @param   buf      : 存放读出字节.
 * @return  成功返回0，失败返回1.
 * @date    2025-12-25.
 * @example soft_i2c_read_byte(&cfg, 0x75, &data);   //读 I2C地址为 cfg->addr 器件寄存器0x75
 *************************************************************************/
u8 soft_i2c_read_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *buf)
{
    soft_i2c_start(cfg);
    soft_i2c_send_byte(cfg, ((cfg->addr) << 1));  // 发送从机地址
    if (soft_i2c_wait_ack(cfg))                 // 如果从机未应答则数据发送失败
    {
        soft_i2c_stop(cfg);
        return 1;
    }
    soft_i2c_send_byte(cfg, reg);               // 发送寄存器地址
    soft_i2c_wait_ack(cfg);
    soft_i2c_start(cfg);
    soft_i2c_send_byte(cfg, (u8)(((cfg->addr) << 1) + 1));    // 进入接收模式
    soft_i2c_wait_ack(cfg);
    *buf = soft_i2c_read_byte_internal(cfg, 0);
    soft_i2c_stop(cfg);                         // 产生一个停止条件
    return 0;
}

/*************************************************************************
 * @code    u8 soft_i2c_write_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 data_t)
 * @brief   模拟I2C写指定设备 指定寄存器的一个值.
 * @param   cfg    : 软件I2C配置参数.
 * @param   reg    : 目标寄存器.
 * @param   data_t : 写入的数据.
 * @return  成功返回0，失败返回1.
 * @date    2025-12-25.
 * @example soft_i2c_write_byte(&cfg, 0X6B, 0X80); //I2C地址为 cfg->addr 器件（MPU6050）寄存器0x6B 写入0x80
 *************************************************************************/
u8 soft_i2c_write_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 data_t)
{
    soft_i2c_start(cfg);
    soft_i2c_send_byte(cfg, ((cfg->addr) << 1));  // 发送从机地址
    if (soft_i2c_wait_ack(cfg))
    {
        soft_i2c_stop(cfg);
        return 1; // 从机地址写入失败
    }
    soft_i2c_send_byte(cfg, reg);               // 发送寄存器地址
    soft_i2c_wait_ack(cfg);
    soft_i2c_send_byte(cfg, data_t);
    if (soft_i2c_wait_ack(cfg))
    {
        soft_i2c_stop(cfg);
        return 1;       // 数据写入失败
    }
    soft_i2c_stop(cfg); // 产生一个停止条件

    return 0;
}

/*************************************************************************
 * @code    u8 soft_i2c_read_n_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *data_t, u8 length)
 * @brief   模拟I2C读取指定设备 指定寄存器的n个值.
 * @param   cfg    : 软件I2C配置参数.
 * @param   reg    : 目标寄存器.
 * @param   data_t : 存放读出数据.
 * @param   length : 读取长度.
 * @return  成功返回0，失败返回1.
 * @date    2025-12-25.
 * @example soft_i2c_read_n_byte(&cfg, 0X3B, &data, 14); // 读 14 个字节
 *************************************************************************/
u8 soft_i2c_read_n_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *data_t, u8 length)
{
    u8 count = 0;
    u8 temp;
    soft_i2c_start(cfg);
    soft_i2c_send_byte(cfg, ((cfg->addr) << 1)); // 发送从机地址
    if (soft_i2c_wait_ack(cfg))
    {
        soft_i2c_stop(cfg);
        return 1; // 从机地址写入失败
    }
    soft_i2c_send_byte(cfg, reg); // 发送寄存器地址
    soft_i2c_wait_ack(cfg);
    soft_i2c_start(cfg);
    soft_i2c_send_byte(cfg, (u8)(((cfg->addr) << 1) + 1)); // 进入接收模式
    soft_i2c_wait_ack(cfg);
    for (count = 0; count < length; count++)
    {
        if (count != (length - 1))
            temp = soft_i2c_read_byte_internal(cfg, 1); // 带ACK的读数据
        else
            temp = soft_i2c_read_byte_internal(cfg, 0); // 最后一个字节NACK

        data_t[count] = temp;
    }
    soft_i2c_stop(cfg); // 产生一个停止条件
    return 0;
}

/*************************************************************************
 * @code    u8 soft_i2c_write_slave_reg_n_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *data_t, u8 length)
 * @brief   模拟I2C写指定设备 指定寄存器的n个值.
 * @param   cfg    : 软件I2C配置参数.
 * @param   reg    : 目标寄存器.
 * @param   data_t : 存放写入数据.
 * @param   length : 写入长度.
 * @return  成功返回0，失败返回1.
 * @date    2025-12-25.
 * @example soft_i2c_write_slave_reg_n_byte(&cfg, 0X6B, 0X80, 1); // 读 14 个字节
 *************************************************************************/
u8 soft_i2c_write_n_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *data_t, u8 length)
{
    u8 count = 0;
    soft_i2c_start(cfg);
    soft_i2c_send_byte(cfg, ((cfg->addr) << 1)); // 发送从机地址
    if (soft_i2c_wait_ack(cfg))
    {
        soft_i2c_stop(cfg);
        return 1; // 从机地址写入失败
    }
    soft_i2c_send_byte(cfg, reg); // 发送寄存器地址
    soft_i2c_wait_ack(cfg);
    for (count = 0; count < length; count++)
    {
        soft_i2c_send_byte(cfg, data_t[count]);
        if (soft_i2c_wait_ack(cfg)) // 每一个字节都要等从机应答
        {
            soft_i2c_stop(cfg);
            return 1; // 数据写入失败
        }
    }
    soft_i2c_stop(cfg); // 产生一个停止条件

    return 0;
}
