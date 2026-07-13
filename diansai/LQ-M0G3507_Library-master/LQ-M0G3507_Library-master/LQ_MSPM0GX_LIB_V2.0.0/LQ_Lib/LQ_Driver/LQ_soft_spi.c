/*******************************************************************************
 * @file                LQ_soft_spi.c
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
#include "LQ_soft_spi.h"

/* SCK 操作宏 */
#define Soft_SPI_SCK_H(n)					LQ_GPIO_WritePin((n)->sck, 1)
#define Soft_SPI_SCK_L(n)					LQ_GPIO_WritePin((n)->sck, 0)

/* MISO 读取宏 */
#define Soft_SPI_MISO_READ(n)				LQ_GPIO_ReadPin((n)->miso)

/* MOSI 操作宏 */
#define Soft_SPI_MOSI_H(n)					LQ_GPIO_WritePin((n)->mosi, 1)
#define Soft_SPI_MOSI_L(n)					LQ_GPIO_WritePin((n)->mosi, 0)

/* CS 操作宏 */
#define Soft_SPI_CS_H(n)					LQ_GPIO_WritePin((n)->cs, 1)
#define Soft_SPI_CS_L(n)					LQ_GPIO_WritePin((n)->cs, 0)

/*************************************************************************
 * @code    void LQ_Soft_SPI_Init(LQConfig_SoftSPI_InitTypeDef_t *cfg)
 *
 * @brief   模拟SPI初始化.
 * @param   cfg : SPI配置参数.
 * @return  成功返回0, 失败返回1.
 *
 * @note    初始化时调用
 *************************************************************************/
void LQ_Soft_SPI_Init(LQConfig_SoftSPI_InitTypeDef_t *cfg)
{
    if (cfg == NULL)
        return;
	LQConfig_GPIO_InitTypeDef_t gpio_output = {
		.Mode  = GPIO_MODE_OUTPUT_PP,
		.Pull  = GPIO_RESISTOR_NO_PULL,
		.Speed = GPIO_SPEED_HIGH,
	};
	LQConfig_GPIO_InitTypeDef_t gpio_input = {
		.Mode  = GPIO_MODE_INPUT,
		.Pull  = GPIO_RESISTOR_NO_PULL,
		.Speed = GPIO_SPEED_HIGH,
	};
	LQ_GPIO_Init(cfg->sck , &gpio_output);
	LQ_GPIO_Init(cfg->miso, &gpio_input);
	LQ_GPIO_Init(cfg->mosi, &gpio_output);
	LQ_GPIO_Init(cfg->cs  , &gpio_output);

    Soft_SPI_SCK_H(cfg); // 片选拉高
}

/*************************************************************************
 * @code    static void LQ_Soft_SPI_ReadWrite(const LQConfig_SoftSPI_InitTypeDef_t *cfg, uint8_t *lqbuff, uint16_t len)
 *
 * @brief   模拟SPI读写数据及长度.
 * @param   cfg   : SPI配置参数.
 * @param   lqbuf : 数据指针.
 * @param   len   : 长度.
 * @return  none.
 *************************************************************************/
static void LQ_Soft_SPI_ReadWrite(const LQConfig_SoftSPI_InitTypeDef_t *cfg, uint8_t *lqbuff, uint16_t len)
{
    uint8_t i;
    if ((lqbuff == NULL) || (SOFT_SPI_MODE_3 < cfg->mode))
        return;
    Soft_SPI_CS_L(cfg); // 拉低片选
    if ((SOFT_SPI_MODE_0 == cfg->mode) || (SOFT_SPI_MODE_1 == cfg->mode))
        Soft_SPI_SCK_L(cfg); // 初始SCK设为低(CPOL=0)
    else
        Soft_SPI_SCK_H(cfg); // 初始SCK设为高(CPOL=1)
    do
    {
        for (i = 0; i < 8; i++)
        {
            if ((*lqbuff) >= 0x80)
                Soft_SPI_MOSI_H(cfg);
            else
                Soft_SPI_MOSI_L(cfg);
            if (SOFT_SPI_MODE_0 == cfg->mode)
            {
                Soft_SPI_SCK_H(cfg); // 产生一个上升沿
                (*lqbuff) = (*lqbuff) << 1;
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg); // 上升沿采样MISO(CPHA=0)
                Soft_SPI_SCK_L(cfg);                   // 产生一个下降沿
            }
            else if (SOFT_SPI_MODE_1 == cfg->mode)
            {
                Soft_SPI_SCK_H(cfg); // 产生一个上升沿
                (*lqbuff) = (*lqbuff) << 1;
                Soft_SPI_SCK_L(cfg);                   // 产生一个下降沿
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg); // 下降沿采样MISO(CPHA=1)
            }
            else if (SOFT_SPI_MODE_2 == cfg->mode)
            {
                Soft_SPI_SCK_L(cfg); // 产生一个下降沿
                (*lqbuff) = (*lqbuff) << 1;
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg); // 下降沿采样MISO(CPHA=0)
                Soft_SPI_SCK_H(cfg);                   // 产生一个上升沿
            }
            else
            {
                Soft_SPI_SCK_L(cfg); // 产生一个下降沿
                (*lqbuff) = (*lqbuff) << 1;
                Soft_SPI_SCK_H(cfg);                   // 产生一个上升沿
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg); // 上升沿采样MISO(CPHA=1)
            }
        }
        lqbuff++;
    } while (--len);
    Soft_SPI_CS_H(cfg); // 拉高片选
}

/*************************************************************************
 * @code    uint8_t LQ_Soft_SPI_ReadByte(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg)
 *
 * @brief   模拟SPI从设备读取一字节数据.
 * @param   cfg : SPI配置参数.
 * @param   reg : 设备起始地址.
 * @return  读取到的数据.
 *************************************************************************/
uint8_t LQ_Soft_SPI_ReadByte(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg)
{
    uint8_t buff[2] = {0};
    if (SOFT_SPI_MODE_3 < cfg->mode)
        return 0;
    buff[0] = reg | 0x80;
    LQ_Soft_SPI_ReadWrite(cfg, buff, 2);
    return buff[1];
}

/*************************************************************************
 * @code    void LQ_Soft_SPI_Read(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf, uint16_t len)
 *
 * @brief   模拟SPI从设备读取 n 字节数据.
 * @param   cfg : SPI配置参数.
 * @param   reg : 设备起始地址.
 * @param   buf : 数据存放地址.
 * @param   len : 待写入的数据长度.
 * @return  none.
 *************************************************************************/
void LQ_Soft_SPI_Read(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t rx_buf[SOFT_SPI_MAX_TRANS_LEN] = {0}, i;
    if ((buf == NULL) || (len == 0) || (SOFT_SPI_MODE_3 < cfg->mode))
        return;
    if (len > (SOFT_SPI_MAX_TRANS_LEN - 1)) // 防止数组越界
        len = SOFT_SPI_MAX_TRANS_LEN - 1;
    rx_buf[0] = reg | 0x80;
    LQ_Soft_SPI_ReadWrite(cfg, rx_buf, len + 1);
    for (i = 0; i < len; i++)
    {
        buf[i] = rx_buf[i + 1];
    }
}

/*************************************************************************
 * @code    void soft_spi_write_byte(const Soft_SPI_cfg_t *cfg, const u8 reg, u8 value)
 * @brief   模拟SPI向设备写入一字节数据.
 * @param   reg   : 设备起始地址.
 * @param   reg   : 设备起始地址.
 * @param   value : 待写入的数据.
 * @return  none.
 * @date    2025-12-26.
 *************************************************************************/
void LQ_Soft_SPI_WriteByte(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t value)
{
    uint8_t buff[2];
    if (SOFT_SPI_MODE_3 < cfg->mode)
        return;
    buff[0] = reg & 0x7f; // 先发送寄存器
    buff[1] = value;      // 再发送数据
    LQ_Soft_SPI_ReadWrite(cfg, buff, 2);
}

/*************************************************************************
 * @code    void soft_spi_write_n_byte(const Soft_SPI_cfg_t *cfg, const u8 reg, u8 *buf, u16 len)
 * @brief   模拟SPI向设备写入 n 字节数据.
 * @param   cfg : SPI配置参数.
 * @param   reg : 设备起始地址.
 * @param   buf : 待写入的数据.
 * @param   len : 待写入的数据长度.
 * @return  none.
 * @date    2025-12-26.
 *************************************************************************/
void LQ_Soft_SPI_Write(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t tx_buf[SOFT_SPI_MAX_TRANS_LEN] = {0}, i;
    if ((buf == NULL) || (len == 0) || (SOFT_SPI_MODE_3 < cfg->mode))
        return;
    if (len > (SOFT_SPI_MAX_TRANS_LEN - 1)) // 防止数组越界
        len = SOFT_SPI_MAX_TRANS_LEN - 1;
    tx_buf[0] = reg & 0x7f;
    for (i = 0; i < len; i++)
    {
        tx_buf[i + 1] = buf[i];
    }
    LQ_Soft_SPI_ReadWrite(cfg, tx_buf, len + 1);
}
