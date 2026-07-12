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
 【SYS CLK】 40 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
#ifndef __LQ_IIC_VL53_H_
#define __LQ_IIC_VL53_H_

#include "config.h"
#include "AI8051U_GPIO.h"

/*VL53软件IIC 管脚，用户可修改 */
#define  VL53_SCL_PIN                           P2_4   // IIC时钟引脚宏定义
#define  VL53_SDA_PIN                           P2_3   // IIC数据引脚宏定义

/*  vl53 基础寄存器宏 */
#define VL53_STAR                                   0x02     //0x02 连续测量模式    0x01 单次测量模式
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xc0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID      0xc2
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD   0x50
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70
#define VL53L0X_REG_SYSRANGE_START                  0x00    // 开始测量
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS         0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS             0x14
#define VL53_REG_DIS                                0x1E
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS        0x8a
#define VL53ADDR                                    0x29    //0x52   默认地址
#define VL53NEWADDR                                 0x30    //VL53新地址


void VL53_Write_Byte(u8 reg, u8 val);
void VL53_Read_nByte(u8 reg, u8 length, u8 *dat);
void VL53_Soft_I2C_Init();


#endif

