/*******************************************************************************
 * @file                LQ_soft_i2c.c
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
 * @author				wuwu	(接口层编写)
 * @author              LQ_012	(优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include "LQ_soft_i2c.h"

/* SCL 操作宏 */
#define Soft_I2C_SCL_H(n)				LQ_GPIO_WritePin((n)->scl, 1)
#define Soft_I2C_SCL_L(n)				LQ_GPIO_WritePin((n)->scl, 0)

/* SDA 操作宏 */
#define Soft_I2C_SDA_H(n)				LQ_GPIO_WritePin((n)->sda, 1)
#define Soft_I2C_SDA_L(n)				LQ_GPIO_WritePin((n)->sda, 0)

/* SDA 读取宏 */
#define Soft_I2C_SDA_READ(n)			LQ_GPIO_ReadPin((n)->sda)

/*************************************************************************
 * @code    	void i2c_delay()
 *
 * @brief   	模拟I2C延时.
 * @param   	us : 延时时间，单位：微秒.
 * @return  	none.
 *
 * @note    	模拟I2C内部使用.
 *************************************************************************/
static void i2c_delay(void)
{
    delay_us(2); // 延时2微秒
}

/*************************************************************************
 * @code    	void LQ_Soft_I2C_Init(LQConfig_SoftI2C_InitTypeDef_t *cfg)
 *
 * @brief   	模拟I2C初始化.
 * @param   	cfg : 软件I2C配置参数.
 * @return  	none.
 *
 * @example		LQConfig_SoftI2C_InitTypeDef_t cfg = {
 *  				.scl  = GPIO_Pin_A_8,
 *  				.sda  = GPIO_Pin_B_7,
 *  				.addr = 0x68,
 *  			};
 *				LQ_Soft_I2C_Init(&cfg);
 *************************************************************************/
void LQ_Soft_I2C_Init(LQConfig_SoftI2C_InitTypeDef_t *cfg)
{
    // 初始化SCL和SDA引脚
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
		.Mode  = GPIO_MODE_OUTPUT_OD,
		.Pull  = GPIO_RESISTOR_PULL_UP,
		.Speed = GPIO_SPEED_HIGH,
	};
	LQ_GPIO_Init(cfg->scl, &gpio_init);
	LQ_GPIO_Init(cfg->sda, &gpio_init);
	
	Soft_I2C_SCL_H(cfg);
	Soft_I2C_SDA_H(cfg);
}

/*************************************************************************
 * @code    	void LQ_Soft_I2C_Start(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
 *
 * @brief   	模拟I2C起始信号.
 * @param   	cfg : 软件I2C配置参数.
 * @return  	none.
 *
 * @note    	模拟I2C内部使用.
 *************************************************************************/
void LQ_Soft_I2C_Start(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
{
    Soft_I2C_SDA_H(cfg);
    Soft_I2C_SCL_H(cfg);
    i2c_delay();
    i2c_delay();
    Soft_I2C_SDA_L(cfg);   // START:when CLK is high,DATA change form high to low
    i2c_delay();
    Soft_I2C_SCL_L(cfg);   // 钳住I2C总线，准备发送或接收数据
    i2c_delay();
}

/*************************************************************************
 * @code    	void LQ_Soft_I2C_Stop(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
 *
 * @brief   	模拟I2C停止信号.
 * @param   	cfg : 软件I2C配置参数.
 * @return  	none.
 *
 * @note    	模拟I2C内部使用.
 *************************************************************************/
void LQ_Soft_I2C_Stop(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
{
    // i2c_delay();
    Soft_I2C_SCL_L(cfg);
    Soft_I2C_SDA_L(cfg);   // STOP:when CLK is high DATA change form low to high
    i2c_delay();
    Soft_I2C_SCL_H(cfg);
    i2c_delay();
    Soft_I2C_SDA_H(cfg);   // 发送I2C总线结束信号
    i2c_delay();
}

/*************************************************************************
 * @code    	uint8_t LQ_Soft_I2C_Wait_ACK(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
 *
 * @brief   	模拟I2C等待应答信号.
 * @param   	cfg : 软件I2C配置参数.
 * @return  	none.
 *
 * @note    	模拟I2C内部使用 有效应答：从机第9个 SCL=0 时 SDA 被从机拉低,并且 SCL = 1时 SDA依然为低.
 *************************************************************************/
uint8_t LQ_Soft_I2C_Wait_ACK(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
{
    uint8_t ucErrTime = 0;
    Soft_I2C_SCL_L(cfg);
    Soft_I2C_SDA_H(cfg);
    i2c_delay();
    Soft_I2C_SCL_H(cfg);
    i2c_delay();
    while (Soft_I2C_SDA_READ(cfg))
    {
        ucErrTime++;
        if (ucErrTime > 100)
        {
            LQ_Soft_I2C_Stop(cfg);
            return 1;
        }
    }
    Soft_I2C_SCL_L(cfg); // 时钟输出0
    i2c_delay();
    return 0;
}

/*************************************************************************
 * @code    	void LQ_Soft_I2C_ACK(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
 *
 * @brief   	模拟I2C产生ACK应答.
 * @param   	cfg : 软件I2C配置参数.
 * @return  	none.
 *
 * @note    	内部调用 主机接收完一个字节数据后，主机产生的ACK通知从机一个字节数据已正确接收.
 *************************************************************************/
void LQ_Soft_I2C_ACK(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
{
    Soft_I2C_SCL_L(cfg);
    Soft_I2C_SDA_L(cfg);
    i2c_delay();
    Soft_I2C_SCL_H(cfg);
    i2c_delay();
    Soft_I2C_SCL_L(cfg);
    Soft_I2C_SDA_H(cfg);
}

/*************************************************************************
 * @code    	void LQ_Soft_I2C_NACK(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
 *
 * @brief   	模拟I2C不产生ACK应答.
 * @param   	cfg : 软件I2C配置参数.
 * @return  	none.
 *
 * @note    	内部调用 主机接收完最后一个字节数据后，主机产生的NACK通知从机发送结束，释放SDA,以便主机产生停止信号.
 *************************************************************************/
void LQ_Soft_I2C_NACK(const LQConfig_SoftI2C_InitTypeDef_t *cfg)
{
    Soft_I2C_SCL_L(cfg);
    Soft_I2C_SDA_H(cfg);
    i2c_delay();
    Soft_I2C_SCL_H(cfg);
    i2c_delay();
    Soft_I2C_SCL_L(cfg);
    Soft_I2C_SDA_H(cfg);
}

/*************************************************************************
 * @code    	void LQ_Soft_I2C_WriteData(const LQConfig_SoftI2C_InitTypeDef_t *cfg, uint8_t data_t)
 *
 * @brief   	模拟I2C发送一个字节.
 * @param   	cfg   : 软件I2C配置参数.
 * @param   	ndata : 发送的字节.
 * @return  	none.
 *
 * @example 	LQ_Soft_I2C_WriteData(&cfg, 0x12); // 模拟I2C发送一个字节.
 *************************************************************************/
void LQ_Soft_I2C_WriteData(const LQConfig_SoftI2C_InitTypeDef_t *cfg, uint8_t data_t)
{
    uint8_t t;
    Soft_I2C_SCL_L(cfg);   // 拉低时钟开始数据传输
    for (t = 0; t < 8; t++)
    {
        if (data_t & 0x80) {
            Soft_I2C_SDA_H(cfg);
        } else {
            Soft_I2C_SDA_L(cfg);
        }
        Soft_I2C_SCL_H(cfg);
        i2c_delay();
        data_t <<= 1;
        i2c_delay();
        Soft_I2C_SCL_L(cfg);
        i2c_delay();
    }
    i2c_delay();
}

/*************************************************************************
 * @code    	uint8_t LQ_Soft_I2C_ReadByte_Internal(const LQConfig_SoftI2C_InitTypeDef_t *cfg, uint8_t ack)
 *
 * @brief   	模拟I2C读取一个字节(内部使用).
 * @param   	cfg : 软件I2C配置参数.
 * @param   	ack : 为 1 时，主机数据还没接收完，为0 时主机数据已全部接收完成.
 * @return  	接收到的字节.
 *
 * @note 		内部使用.
 *************************************************************************/
uint8_t LQ_Soft_I2C_ReadByte_Internal(const LQConfig_SoftI2C_InitTypeDef_t *cfg, uint8_t ack)
{
    uint8_t i, receive = 0;
    for (i = 0; i < 8; i++)
    {
        Soft_I2C_SCL_L(cfg);
        i2c_delay();
        Soft_I2C_SCL_H(cfg);
        receive <<= 1;
        if (Soft_I2C_SDA_READ(cfg))
            receive++; // 从机发送的电平
        i2c_delay();
    }
    if (ack)
        LQ_Soft_I2C_ACK(cfg);     // 发送ACK
    else
        LQ_Soft_I2C_NACK(cfg);    // 发送nACK
    return receive;
}

/*************************************************************************
 * @code    	uint8_t LQ_Soft_I2C_RecvByte(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf)
 *
 * @brief   	模拟I2C读取指定设备 指定寄存器的一个值.
 * @param   	cfg      : 软件I2C配置参数.
 * @param   	reg      : 目标寄存器.
 * @param   	buf      : 存放读出字节.
 * @return  	成功返回0，失败返回1.
 *
 * @example 	LQ_Soft_I2C_RecvByte(&cfg, 0x75, &data);   //读 I2C地址为 cfg->addr 器件寄存器0x75
 *************************************************************************/
uint8_t LQ_Soft_I2C_RecvByte(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf)
{
    LQ_Soft_I2C_Start(cfg);
    LQ_Soft_I2C_WriteData(cfg, ((cfg->addr) << 1));  // 发送从机地址
    if (LQ_Soft_I2C_Wait_ACK(cfg))                 // 如果从机未应答则数据发送失败
    {
        LQ_Soft_I2C_Stop(cfg);
        return 1;
    }
    LQ_Soft_I2C_WriteData(cfg, reg);               // 发送寄存器地址
    LQ_Soft_I2C_Wait_ACK(cfg);
    LQ_Soft_I2C_Start(cfg);
    LQ_Soft_I2C_WriteData(cfg, (uint8_t)(((cfg->addr) << 1) + 1));    // 进入接收模式
    LQ_Soft_I2C_Wait_ACK(cfg);
    *buf = LQ_Soft_I2C_ReadByte_Internal(cfg, 0);
    LQ_Soft_I2C_Stop(cfg);                         // 产生一个停止条件
    return 0;
}

/*************************************************************************
 * @code    	uint8_t LQ_Soft_I2C_SendByte(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t data_t)
 *
 * @brief   	模拟I2C写指定设备 指定寄存器的一个值.
 * @param   	cfg    : 软件I2C配置参数.
 * @param   	reg    : 目标寄存器.
 * @param   	data_t : 写入的数据.
 * @return  	成功返回0，失败返回1.
 *
 * @example 	LQ_Soft_I2C_SendByte(&cfg, 0X6B, 0X80); //I2C地址为 cfg->addr 器件（MPU6050）寄存器0x6B 写入0x80
 *************************************************************************/
uint8_t LQ_Soft_I2C_SendByte(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t data_t)
{
    LQ_Soft_I2C_Start(cfg);
    LQ_Soft_I2C_WriteData(cfg, ((cfg->addr) << 1));  // 发送从机地址
    if (LQ_Soft_I2C_Wait_ACK(cfg)
    )
    {
        LQ_Soft_I2C_Stop(cfg);
        return 1; // 从机地址写入失败
    }
    LQ_Soft_I2C_WriteData(cfg, reg);               // 发送寄存器地址
    LQ_Soft_I2C_Wait_ACK(cfg);
    LQ_Soft_I2C_WriteData(cfg, data_t);
    if (LQ_Soft_I2C_Wait_ACK(cfg)
    )
    {
        LQ_Soft_I2C_Stop(cfg);
        return 1;       // 数据写入失败
    }
    LQ_Soft_I2C_Stop(cfg); // 产生一个停止条件

    return 0;
}

/*************************************************************************
 * @code    	uint8_t LQ_Soft_I2C_RecvBuffer(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *data_t, uint8_t length)
 *
 * @brief   	模拟I2C读取指定设备 指定寄存器的n个值.
 * @param   	cfg    : 软件I2C配置参数.
 * @param   	reg    : 目标寄存器.
 * @param   	data_t : 存放读出数据.
 * @param   	length : 读取长度.
 * @return  	成功返回0，失败返回1.
 *
 * @example 	LQ_Soft_I2C_RecvBuffer(&cfg, 0X3B, &data, 14); // 读 14 个字节
 *************************************************************************/
uint8_t LQ_Soft_I2C_RecvBuffer(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *data_t, uint8_t length)
{
    uint8_t count = 0;
    uint8_t temp;
    LQ_Soft_I2C_Start(cfg);
    LQ_Soft_I2C_WriteData(cfg, ((cfg->addr) << 1)); // 发送从机地址
    if (LQ_Soft_I2C_Wait_ACK(cfg)
    )
    {
        LQ_Soft_I2C_Stop(cfg);
        return 1; // 从机地址写入失败
    }
    LQ_Soft_I2C_WriteData(cfg, reg); // 发送寄存器地址
    LQ_Soft_I2C_Wait_ACK(cfg);
    LQ_Soft_I2C_Start(cfg);
    LQ_Soft_I2C_WriteData(cfg, (uint8_t)(((cfg->addr) << 1) + 1)); // 进入接收模式
    LQ_Soft_I2C_Wait_ACK(cfg);
    for (count = 0; count < length; count++)
    {
        if (count != (length - 1))
            temp = LQ_Soft_I2C_ReadByte_Internal(cfg, 1); // 带ACK的读数据
        else
            temp = LQ_Soft_I2C_ReadByte_Internal(cfg, 0); // 最后一个字节NACK

        data_t[count] = temp;
    }
    LQ_Soft_I2C_Stop(cfg); // 产生一个停止条件
    return 0;
}

/*************************************************************************
 * @code    	uint8_t LQ_Soft_I2C_SendBuffer(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *data_t, uint8_t length)
 *
 * @brief   	模拟I2C写指定设备 指定寄存器的n个值.
 * @param   	cfg    : 软件I2C配置参数.
 * @param   	reg    : 目标寄存器.
 * @param   	data_t : 存放写入数据.
 * @param   	length : 写入长度.
 * @return 		成功返回0，失败返回1.
 *
 * @example 	LQ_Soft_I2C_SendBuffer(&cfg, 0X6B, 0X80, 1); // 读 14 个字节
 *************************************************************************/
uint8_t LQ_Soft_I2C_SendBuffer(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *data_t, uint8_t length)
{
    uint8_t count = 0;
    LQ_Soft_I2C_Start(cfg);
    LQ_Soft_I2C_WriteData(cfg, ((cfg->addr) << 1)); // 发送从机地址
    if (LQ_Soft_I2C_Wait_ACK(cfg)
    )
    {
        LQ_Soft_I2C_Stop(cfg);
        return 1; // 从机地址写入失败
    }
    LQ_Soft_I2C_WriteData(cfg, reg); // 发送寄存器地址
    LQ_Soft_I2C_Wait_ACK(cfg);
    for (count = 0; count < length; count++)
    {
        LQ_Soft_I2C_WriteData(cfg, data_t[count]);
        if (LQ_Soft_I2C_Wait_ACK(cfg)
        ) // 每一个字节都要等从机应答
        {
            LQ_Soft_I2C_Stop(cfg);
            return 1; // 数据写入失败
        }
    }
    LQ_Soft_I2C_Stop(cfg); // 产生一个停止条件

    return 0;
}
