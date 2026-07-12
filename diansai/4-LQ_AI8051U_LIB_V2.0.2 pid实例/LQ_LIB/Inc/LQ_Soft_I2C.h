#ifndef __LQ_SOFT_I2C_X_H
#define __LQ_SOFT_I2C_X_H

#include "config.h"
#include "AI8051U_GPIO.h"

/* 注意 IIC总线规定，IIC空闲时 SCL和SDA都为高电平 最好外部上拉（一定不能下拉） */
/* 模拟 IIC需要注意，IIC地址左移一位 例如MPU6050 模拟就是地址 0xD0 */

// 软件 I2C 配置结构体
typedef struct
{
    Pin_list    scl;    // 软件 I2C 时钟线
    Pin_list    sda;    // 软件 I2C 数据线
    u8          addr;   // 设备地址
} Soft_I2C_cfg_t;

#define Soft_I2C_SCL_Out(n)     gpio_init_pin(n, GPIO_Mode_Out_PP)      /*!< SCL 输出模式 */
#define Soft_I2C_SDA_Out(n)     gpio_init_pin(n, GPIO_Mode_Out_PP)      /*!< SDA 输出模式 */
#define Soft_I2C_SDA_In(n)      gpio_init_pin(n, GPIO_Mode_IN_FLOATING) /*!< SDA 输入模式 */

#define Soft_I2C_SCL_H(n)       gpio_write_pin(n, HIGH)                  /*!< SCL 拉高 */
#define Soft_I2C_SCL_L(n)       gpio_write_pin(n, LOW)                   /*!< SCL 拉低 */

#define Soft_I2C_SDA_H(n)       gpio_write_pin(n, HIGH)                  /*!< SDA 拉高 */
#define Soft_I2C_SDA_L(n)       gpio_write_pin(n, LOW)                /*!< SDA 拉低 */

#define Soft_I2C_SDA_READ(n)    gpio_read_pin(n)                        /*!< 读取 SDA 状态 */

//========================================================================
//                            外部调用函数
//========================================================================

void soft_i2c_init(Soft_I2C_cfg_t *cfg);

u8 soft_i2c_read_byte   (const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *buf);
u8 soft_i2c_read_n_byte (const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *data_t, u8 length);

u8 soft_i2c_write_byte  (const Soft_I2C_cfg_t *cfg, const u8 reg, u8  data_t);
u8 soft_i2c_write_n_byte(const Soft_I2C_cfg_t *cfg, const u8 reg, u8 *data_t, u8 length);

#endif
