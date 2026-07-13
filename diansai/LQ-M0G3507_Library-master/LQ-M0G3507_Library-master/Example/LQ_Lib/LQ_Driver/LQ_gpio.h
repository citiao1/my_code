/*******************************************************************************
 * @file                LQ_gpio.h
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
#ifndef __LQ_GPIO_H__
#define __LQ_GPIO_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

/**
 * @brief       MSPM0G3507 GPIO 引脚枚举
 * 
 * @note        IO顺序按照" mspm0g350x.h" 中 enum IOMUX_PINCMIOMUX 排序
 * 
 * @note        GPIO_Pin_A_3 | GPIO_Pin_A_4 | GPIO_Pin_A_5 | GPIO_Pin_A_6
 *              这几个引脚用作外部晶振输入,不要使用
 *              这几个引脚用作外部晶振输入,不要使用
 *              这几个引脚用作外部晶振输入,不要使用
 */
typedef enum {

    GPIO_Pin_A_0 , GPIO_Pin_A_1 , GPIO_Pin_A_28, GPIO_Pin_A_29, GPIO_Pin_A_30, GPIO_Pin_A_31,

    GPIO_Pin_A_2 , GPIO_Pin_A_3 , GPIO_Pin_A_4 , GPIO_Pin_A_5 , GPIO_Pin_A_6 , GPIO_Pin_B_0 ,
    
    GPIO_Pin_B_1 , GPIO_Pin_A_7 , GPIO_Pin_B_2 , GPIO_Pin_B_3 , GPIO_Pin_B_4 , GPIO_Pin_B_5 ,
    
    GPIO_Pin_A_8 , GPIO_Pin_A_9 , GPIO_Pin_A_10, GPIO_Pin_A_11, GPIO_Pin_B_6 , GPIO_Pin_B_7 ,
    
    GPIO_Pin_B_8 , GPIO_Pin_B_9 , GPIO_Pin_B_10, GPIO_Pin_B_11, GPIO_Pin_B_12, GPIO_Pin_B_13,
    
    GPIO_Pin_B_14, GPIO_Pin_B_15, GPIO_Pin_B_16, GPIO_Pin_A_12, GPIO_Pin_A_13, GPIO_Pin_A_14,
    
    GPIO_Pin_A_15, GPIO_Pin_A_16, GPIO_Pin_A_17, GPIO_Pin_A_18, GPIO_Pin_A_19, GPIO_Pin_A_20,
    
    GPIO_Pin_B_17, GPIO_Pin_B_18, GPIO_Pin_B_19, GPIO_Pin_A_21, GPIO_Pin_A_22, GPIO_Pin_B_20,
    
    GPIO_Pin_B_21, GPIO_Pin_B_22, GPIO_Pin_B_23, GPIO_Pin_B_24, GPIO_Pin_A_23, GPIO_Pin_A_24,
    
    GPIO_Pin_A_25, GPIO_Pin_B_25, GPIO_Pin_B_26, GPIO_Pin_B_27, GPIO_Pin_A_26, GPIO_Pin_A_27,

    GPIO_Pin_ALL_NUM

} LQEnum_GPIO_Pin_t;

/*!
 * @enum	GPIO 模式
 *
 */
typedef enum {
    /* GPIO 配置成推挽输出模式 */
    GPIO_MODE_OUTPUT_PP,

    /* GPIO 配置成开漏输出模式 */
    GPIO_MODE_OUTPUT_OD,

    /* GPIO 配置成输入模式 */
    GPIO_MODE_INPUT
} LQEnum_GPIO_Mode_t;

/*!
 * @enum	GPIO 内部电阻配置
 *
 */
typedef enum {
    /* 浮空模式 */
    GPIO_RESISTOR_NO_PULL =
        (IOMUX_PINCM_PIPU_DISABLE | IOMUX_PINCM_PIPD_DISABLE),

    /* 上拉模式 */
    GPIO_RESISTOR_PULL_UP =
        (IOMUX_PINCM_PIPU_ENABLE | IOMUX_PINCM_PIPD_DISABLE),

    /* 下拉模式 */
    GPIO_RESISTOR_PULL_DOWN =
        (IOMUX_PINCM_PIPU_DISABLE | IOMUX_PINCM_PIPD_ENABLE)
} LQEnum_GPIO_Pull_t;

/*! 
 * @enum	GPIO 驱动能力
 *
 * @note	只有 HDIO 和 HSIO 有效, 其他 I/O 配置无事发生
 */
typedef enum {
    /*低驱*/
    GPIO_SPEED_LOW = IOMUX_PINCM_DRV_DRVVAL0,      

    /*高驱*/
    GPIO_SPEED_HIGH = IOMUX_PINCM_DRV_DRVVAL1           
} LQEnum_GPIO_Speed_t;

/**
 * @brief       GPIO 配置结构体
 * 
 * @param       Mode                GPIO模式选择
 * @param       Pull                上下拉设置
 * @param       Speed               IO口驱动能力
 */
typedef struct {
	
    /* GPIO模式选择 */
    LQEnum_GPIO_Mode_t		Mode;

    /* GPIO上下拉设置 */
    LQEnum_GPIO_Pull_t		Pull;

    /* MSPM0每个IO口能力固定, 只有HDIO和HSIO可以控制驱动能力 */
    LQEnum_GPIO_Speed_t		Speed;

} LQConfig_GPIO_InitTypeDef_t;

extern GPIO_Regs *LQ_GPIO_Regs[2];

extern const uint8_t LQ_GPIO_MAP[GPIO_Pin_ALL_NUM][2];

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

void LQ_GPIO_Init(LQEnum_GPIO_Pin_t Pin, LQConfig_GPIO_InitTypeDef_t *GPIO_Init);	/*! @brief	GPIO初始化 */

void LQ_GPIO_Pin_Init(LQEnum_GPIO_Pin_t Pin_t,LQEnum_GPIO_Mode_t Mode_t,LQEnum_GPIO_Pull_t Pull_t); /* @brief	GPIO_Pin初始化 */

void LQ_GPIO_WritePin(LQEnum_GPIO_Pin_t Pin, uint8_t Value);						/*! @brief	设置 GPIO 写入高低电平 */

void LQ_GPIO_TogglePin(LQEnum_GPIO_Pin_t Pin);										/*! @brief	设置 GPIO 电平翻转 */

int  LQ_GPIO_ReadPin(LQEnum_GPIO_Pin_t Pin);										/*! @brief	读取 GPIO 高低电平 */

#endif
