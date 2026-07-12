#include "include.h"

/*************************************************************************
 * @code    void soft_spi_init(Soft_SPI_cfg_t *cfg)
 * @brief   模拟SPI初始化.
 * @param   cfg : SPI配置参数.
 * @return  成功返回0, 失败返回1.
 * @date    2025-12-26.
 * @note    初始化时调用
 *************************************************************************/
void soft_spi_init(Soft_SPI_cfg_t *cfg)
{
    if (cfg == NULL)
        return;
    Soft_SPI_SCK_Out(cfg->sck);
    Soft_SPI_MISO_In(cfg->miso);
    Soft_SPI_MOSI_Out(cfg->mosi);
    Soft_SPI_CS_Out(cfg->cs);

    Soft_SPI_SCK_L(cfg->sck); // 片选拉高
}

/*************************************************************************
 * @code    void soft_spi_read_write_n_byte(const Soft_SPI_cfg_t *cfg, u8 *lqbuff, u16 len)
 * @brief   模拟SPI读写数据及长度.
 * @param   cfg   : SPI配置参数.
 * @param   lqbuf : 数据指针.
 * @param   len   : 长度.
 * @return  none.
 * @date    2025-12-26.
 *************************************************************************/
void soft_spi_read_write_n_byte(const Soft_SPI_cfg_t *cfg, u8 *lqbuff, u16 len)
{
    u8 i;
    if ((lqbuff == NULL) || (Soft_SPI_Mode_3 < cfg->mode))
        return;
    Soft_SPI_CS_L(cfg->cs); // 拉低片选
    if ((Soft_SPI_Mode_0 == cfg->mode) || (Soft_SPI_Mode_1 == cfg->mode))
        Soft_SPI_SCK_L(cfg->sck); // 初始SCK设为低(CPOL=0)
    else
        Soft_SPI_SCK_H(cfg->sck); // 初始SCK设为高(CPOL=1)
    do
    {
        for (i = 0; i < 8; i++)
        {
            if ((*lqbuff) >= 0x80)
                Soft_SPI_MOSI_H(cfg->mosi);
            else
                Soft_SPI_MOSI_L(cfg->mosi);
            if (Soft_SPI_Mode_0 == cfg->mode)
            {
                Soft_SPI_SCK_H(cfg->sck); // 产生一个上升沿
                (*lqbuff) = (*lqbuff) << 1;
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg->miso); // 上升沿采样MISO(CPHA=0)
                Soft_SPI_SCK_L(cfg->sck);                   // 产生一个下降沿
            }
            else if (Soft_SPI_Mode_1 == cfg->mode)
            {
                Soft_SPI_SCK_H(cfg->sck); // 产生一个上升沿
                (*lqbuff) = (*lqbuff) << 1;
                Soft_SPI_SCK_L(cfg->sck);                   // 产生一个下降沿
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg->miso); // 下降沿采样MISO(CPHA=1)
            }
            else if (Soft_SPI_Mode_2 == cfg->mode)
            {
                Soft_SPI_SCK_L(cfg->sck); // 产生一个下降沿
                (*lqbuff) = (*lqbuff) << 1;
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg->miso); // 下降沿采样MISO(CPHA=0)
                Soft_SPI_SCK_H(cfg->sck);                   // 产生一个上升沿
            }
            else
            {
                Soft_SPI_SCK_L(cfg->sck); // 产生一个下降沿
                (*lqbuff) = (*lqbuff) << 1;
                Soft_SPI_SCK_H(cfg->sck);                   // 产生一个上升沿
                (*lqbuff) |= Soft_SPI_MISO_READ(cfg->miso); // 上升沿采样MISO(CPHA=1)
            }
        }
        lqbuff++;
    } while (--len);
    Soft_SPI_CS_H(cfg->cs); // 拉高片选
}

/*************************************************************************
 * @code    u8 soft_spi_read_byte(const Soft_SPI_cfg_t *cfg, const u8 reg)
 * @brief   模拟SPI从设备读取一字节数据.
 * @param   cfg : SPI配置参数.
 * @param   reg : 设备起始地址.
 * @return  读取到的数据.
 * @date    2025-12-26.
 *************************************************************************/
u8 soft_spi_read_byte(const Soft_SPI_cfg_t *cfg, const u8 reg)
{
    u8 buff[2];
    if (Soft_SPI_Mode_3 < cfg->mode)
        return 0;
    buff[0] = reg | 0x80;
    soft_spi_read_write_n_byte(cfg, buff, 2);
    return buff[1];
}

/*************************************************************************
 * @code    void soft_spi_read_n_byte(const Soft_SPI_cfg_t *cfg, const u8 reg, u8 *buf, u16 len)
 * @brief   模拟SPI从设备读取 n 字节数据.
 * @param   cfg : SPI配置参数.
 * @param   reg : 设备起始地址.
 * @param   buf : 数据存放地址.
 * @param   len : 待写入的数据长度.
 * @return  none.
 * @date    2025-12-26.
 *************************************************************************/
void soft_spi_read_n_byte(const Soft_SPI_cfg_t *cfg, const u8 reg, u8 *buf, u16 len)
{
    u8 rx_buf[SOFT_SPI_MAX_TRANS_LEN] = {0}, i;
    if ((buf == NULL) || (len == 0) || (Soft_SPI_Mode_3 < cfg->mode))
        return;
    if (len > (SOFT_SPI_MAX_TRANS_LEN - 1)) // 防止数组越界
        len = SOFT_SPI_MAX_TRANS_LEN - 1;
    rx_buf[0] = reg | 0x80;
    soft_spi_read_write_n_byte(cfg, rx_buf, len + 1);
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
void soft_spi_write_byte(const Soft_SPI_cfg_t *cfg, const u8 reg, u8 value)
{
    u8 buff[2];
    if (Soft_SPI_Mode_3 < cfg->mode)
        return;
    buff[0] = reg & 0x7f; // 先发送寄存器
    buff[1] = value;      // 再发送数据
    soft_spi_read_write_n_byte(cfg, buff, 2);
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
void soft_spi_write_n_byte(const Soft_SPI_cfg_t *cfg, const u8 reg, u8 *buf, u16 len)
{
    u8 tx_buf[SOFT_SPI_MAX_TRANS_LEN] = {0}, i;
    if ((buf == NULL) || (len == 0) || (Soft_SPI_Mode_3 < cfg->mode))
        return;
    if (len > (SOFT_SPI_MAX_TRANS_LEN - 1)) // 防止数组越界
        len = SOFT_SPI_MAX_TRANS_LEN - 1;
    tx_buf[0] = reg & 0x7f;
    for (i = 0; i < len; i++)
    {
        tx_buf[i + 1] = buf[i];
    }
    soft_spi_read_write_n_byte(cfg, tx_buf, len + 1);
}
