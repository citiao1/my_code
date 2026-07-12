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

#include "LQ_I2C_VL53.h"
#include "include.h"
#include "LQ_Soft_I2C.h"

static Soft_I2C_cfg_t vl53_i2c_cfg; // I2C配置结构体

void VL53_Soft_I2C_Init()
{
    vl53_i2c_cfg.scl = VL53_SCL_PIN; // 设置I2C的SCL引脚
    vl53_i2c_cfg.sda = VL53_SDA_PIN; // 设置I2C的SDA引脚
    vl53_i2c_cfg.addr = VL53ADDR;    // 设置I2C的从机通信地址
    soft_i2c_init(&vl53_i2c_cfg);
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void VL53_Write_Byte(u8 reg, u8 val)
@功能说明：VL53 写1个寄存器 reg：  寄存器 data：  写入数据
@参数说明：dev：    地址
@函数返回：无
@修改时间：2022/03/15
@调用方法：无
@备    注：无
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void VL53_Write_Byte(u8 reg, u8 val)
{
    soft_i2c_write_n_byte(&vl53_i2c_cfg, reg, &val, 1);
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void VL53_Read_nByte(u8 reg, u8 length, u8 *dat)
@功能说明：VL53 写1个寄存器 reg：  寄存器 data：  写入数据
@参数说明：dev：    地址
@函数返回：N字节数据
@修改时间：2022/03/15
@调用方法：无
@备    注：无
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void VL53_Read_nByte(u8 reg, u8 length, u8 *dat)
{ 
   soft_i2c_read_n_byte(&vl53_i2c_cfg, reg, dat, length);
}

