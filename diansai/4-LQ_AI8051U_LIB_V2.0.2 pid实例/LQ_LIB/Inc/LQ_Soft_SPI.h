#ifndef __LQ_SOFT_SPI_H
#define __LQ_SOFT_SPI_H

#include "config.h"

// 软件 SPI 模式设置
typedef enum
{
    Soft_SPI_Mode_0 = 0x00,    // CPOL=0, CPHA=0 (时钟默认低电平，第一个跳变沿采样数据)
    Soft_SPI_Mode_1,                // CPOL=0, CPHA=1 (时钟默认低电平，第二个跳变沿采样数据)
    Soft_SPI_Mode_2,                // CPOL=1, CPHA=0 (时钟默认高电平，第一个跳变沿采样数据)
    Soft_SPI_Mode_3,                // CPOL=1, CPHA=1 (时钟默认高电平，第二个跳变沿采样数据)
} Soft_SPI_Mode_t;

// 软件 SPI 结构体，默认 8 位模式   
typedef struct
{
    Pin_list        sck;              // 软件 SPI 时钟线
    Pin_list        miso;             // 软件 SPI 接收线
    Pin_list        mosi;             // 软件 SPI 发送线
    Pin_list        cs;               // 软件 SPI 片选线
    Soft_SPI_Mode_t mode;             // 软件 SPI 模式
} Soft_SPI_cfg_t;

#define SOFT_SPI_MAX_TRANS_LEN      ( 64 )  // 软件 SPI 一次性最大传输长度

#define Soft_SPI_SCK_Out(n)         gpio_init_pin(n, GPIO_Mode_Out_PP);      /*!< SCL 输出模式 */
#define Soft_SPI_MISO_In(n)         gpio_init_pin(n, GPIO_Mode_IN_FLOATING); /*!< MISO 输入模式 */
#define Soft_SPI_MOSI_Out(n)        gpio_init_pin(n, GPIO_Mode_Out_PP);      /*!< MOSI 输出模式 */
#define Soft_SPI_CS_Out(n)          gpio_init_pin(n, GPIO_Mode_Out_PP);       /*!< CS 输出模式 */

#define Soft_SPI_SCK_H(n)           gpio_write_pin(n, HIGH)                  /*!< SCK 拉高 */
#define Soft_SPI_SCK_L(n)           gpio_write_pin(n, LOW)                   /*!< SCK 拉低 */

#define Soft_SPI_MISO_READ(n)       gpio_read_pin(n)                         /*!< MISO 读取 */

#define Soft_SPI_MOSI_H(n)          gpio_write_pin(n, HIGH)                  /*!< MOSI 拉高 */
#define Soft_SPI_MOSI_L(n)          gpio_write_pin(n, LOW)                  /*!< MOSI 拉低 */

#define Soft_SPI_CS_H(n)            gpio_write_pin(n, HIGH)                   /*!< CS 拉高 */
#define Soft_SPI_CS_L(n)            gpio_write_pin(n, LOW)                    /*!< CS 拉低 */

//========================================================================
//                            外部调用函数
//========================================================================

void soft_spi_init         (Soft_SPI_cfg_t *cfg);

u8   soft_spi_read_byte    (const Soft_SPI_cfg_t *cfg, const u8 reg);
void soft_spi_read_n_byte  (const Soft_SPI_cfg_t *cfg, const u8 reg, u8 *buf, u16 len);

void soft_spi_write_byte   (const Soft_SPI_cfg_t *cfg, const u8 reg, u8 value);
void soft_spi_write_n_byte (const Soft_SPI_cfg_t *cfg, const u8 reg, u8 *buf, u16 len);


#endif
