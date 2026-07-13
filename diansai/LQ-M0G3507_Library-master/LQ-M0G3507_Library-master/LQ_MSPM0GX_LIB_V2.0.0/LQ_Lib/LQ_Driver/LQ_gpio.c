/*******************************************************************************
 * @file                LQ_gpio.c
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
#include "LQ_gpio.h"

/********************************************************************************
 * @note        MSPM0用一个叫做 IOMUX（输入输出多路复用器） 的单元来统一管理所有数字IO
 *              并且它把引脚功能选择和电气特性配置分成了两个独立的步骤.
 * 
 * @note        第一步: 先把IOMUX复用到需要的外设模块上, 如GPIO, UART等.
 * 
 * @note        第二部: 配置对应外设模块对应的功能
 ********************************************************************************/

/**
 * @brief       GPIO 寄存器 映射表
 * 
 * @note
 */
GPIO_Regs *LQ_GPIO_Regs[2] = {GPIOA, GPIOB};

/**
 * @brief       GPIO映射表
 * 
 * @note        IOMUX与GPIO外设上引脚的映射关系
 */
const uint8_t LQ_GPIO_MAP[GPIO_Pin_ALL_NUM][2] = {

    // 端口号 (0=GPIOA, 1=GPIOB)

    {0,  0},  // GPIO_Pin_A_0   -> GPIOA, PIN 0
    {0,  1},  // GPIO_Pin_A_1   -> GPIOA, PIN 1
    {0, 28},  // GPIO_Pin_A_28  -> GPIOA, PIN 28
    {0, 29},  // GPIO_Pin_A_29  -> GPIOA, PIN 29
    {0, 30},  // GPIO_Pin_A_30  -> GPIOA, PIN 30
    {0, 31},  // GPIO_Pin_A_31  -> GPIOA, PIN 31

    {0,  2},  // GPIO_Pin_A_2   -> GPIOA, PIN 2
    {0,  3},  // GPIO_Pin_A_3   -> GPIOA, PIN 3
    {0,  4},  // GPIO_Pin_A_4   -> GPIOA, PIN 4
    {0,  5},  // GPIO_Pin_A_5   -> GPIOA, PIN 5
    {0,  6},  // GPIO_Pin_A_6   -> GPIOA, PIN 6
    {1,  0},  // GPIO_Pin_B_0   -> GPIOB, PIN 0

    {1,  1},  // GPIO_Pin_B_1   -> GPIOB, PIN 1
    {0,  7},  // GPIO_Pin_A_7   -> GPIOA, PIN 7
    {1,  2},  // GPIO_Pin_B_2   -> GPIOB, PIN 2
    {1,  3},  // GPIO_Pin_B_3   -> GPIOB, PIN 3
    {1,  4},  // GPIO_Pin_B_4   -> GPIOB, PIN 4
    {1,  5},  // GPIO_Pin_B_5   -> GPIOB, PIN 5

    {0,  8},  // GPIO_Pin_A_8   -> GPIOA, PIN 8
    {0,  9},  // GPIO_Pin_A_9   -> GPIOA, PIN 9
    {0, 10},  // GPIO_Pin_A_10  -> GPIOA, PIN 10
    {0, 11},  // GPIO_Pin_A_11  -> GPIOA, PIN 11
    {1,  6},  // GPIO_Pin_B_6   -> GPIOB, PIN 6
    {1,  7},  // GPIO_Pin_B_7   -> GPIOB, PIN 7

    {1,  8},  // GPIO_Pin_B_8   -> GPIOB, PIN 8
    {1,  9},  // GPIO_Pin_B_9   -> GPIOB, PIN 9
    {1, 10},  // GPIO_Pin_B_10  -> GPIOB, PIN 10
    {1, 11},  // GPIO_Pin_B_11  -> GPIOB, PIN 11
    {1, 12},  // GPIO_Pin_B_12  -> GPIOB, PIN 12
    {1, 13},  // GPIO_Pin_B_13  -> GPIOB, PIN 13

    {1, 14},  // GPIO_Pin_B_14  -> GPIOB, PIN 14
    {1, 15},  // GPIO_Pin_B_15  -> GPIOB, PIN 15
    {1, 16},  // GPIO_Pin_B_16  -> GPIOB, PIN 16
    {0, 12},  // GPIO_Pin_A_12  -> GPIOA, PIN 12
    {0, 13},  // GPIO_Pin_A_13  -> GPIOA, PIN 13
    {0, 14},  // GPIO_Pin_A_14  -> GPIOA, PIN 14

    {0, 15},  // GPIO_Pin_A_15  -> GPIOA, PIN 15
    {0, 16},  // GPIO_Pin_A_16  -> GPIOA, PIN 16
    {0, 17},  // GPIO_Pin_A_17  -> GPIOA, PIN 17
    {0, 18},  // GPIO_Pin_A_18  -> GPIOA, PIN 18
    {0, 19},  // GPIO_Pin_A_19  -> GPIOA, PIN 19
    {0, 20},  // GPIO_Pin_A_20  -> GPIOA, PIN 20

    {1, 17},  // GPIO_Pin_B_17  -> GPIOB, PIN 17
    {1, 18},  // GPIO_Pin_B_18  -> GPIOB, PIN 18
    {1, 19},  // GPIO_Pin_B_19  -> GPIOB, PIN 19
    {0, 21},  // GPIO_Pin_A_21  -> GPIOA, PIN 21
    {0, 22},  // GPIO_Pin_A_22  -> GPIOA, PIN 22
    {1, 20},  // GPIO_Pin_B_20  -> GPIOB, PIN 20

    {1, 21},  // GPIO_Pin_B_21  -> GPIOB, PIN 21
    {1, 22},  // GPIO_Pin_B_22  -> GPIOB, PIN 22
    {1, 23},  // GPIO_Pin_B_23  -> GPIOB, PIN 23
    {1, 24},  // GPIO_Pin_B_24  -> GPIOB, PIN 24
    {0, 23},  // GPIO_Pin_A_23  -> GPIOA, PIN 23
    {0, 24},  // GPIO_Pin_A_24  -> GPIOA, PIN 24

    {0, 25},  // GPIO_Pin_A_25  -> GPIOA, PIN 25
    {1, 25},  // GPIO_Pin_B_25  -> GPIOB, PIN 25
    {1, 26},  // GPIO_Pin_B_26  -> GPIOB, PIN 26
    {1, 27},  // GPIO_Pin_B_27  -> GPIOB, PIN 27
    {0, 26},  // GPIO_Pin_A_26  -> GPIOA, PIN 26
    {0, 27},  // GPIO_Pin_A_27  -> GPIOA, PIN 27

};

/********************************************************************************
 * @brief       GPIO初始化
 * 
 * @param       Pin				GPIO引脚
 * @param       GPIO_Init		GPIO模式
 * 
 * @example		LQConfig_GPIO_InitTypeDef_t gpio_output = {
 *					.Mode  = GPIO_MODE_OUTPUT_PP,	// 配置输出模式
 *					.Pull  = GPIO_RESISTOR_NO_PULL,	// 配置浮空模式
 *					.Speed = GPIO_SPEED_LOW			// 配置低驱模式
 *				};
 *				LQ_GPIO_Init(GPIO_Pin_A_15, &gpio_output);
 ********************************************************************************/
void LQ_GPIO_Init(LQEnum_GPIO_Pin_t Pin, LQConfig_GPIO_InitTypeDef_t *GPIO_Init)
{
    uint32_t config = IOMUX_PINCM_PC_CONNECTED | ((uint32_t) 0x00000001)
                    | GPIO_Init->Pull | GPIO_Init->Speed;

    switch(GPIO_Init->Mode)
    {
        case GPIO_MODE_OUTPUT_OD: config |= DL_GPIO_HIZ_ENABLE | IOMUX_PINCM_INENA_ENABLE; break;

        case GPIO_MODE_INPUT:     config |= IOMUX_PINCM_INENA_ENABLE; break;

        default: break;
    }

    IOMUX->SECCFG.PINCM[Pin] = config;

    if(GPIO_Init->Mode != GPIO_MODE_INPUT)
    {
        LQ_GPIO_Regs[LQ_GPIO_MAP[Pin][0]]->DOUTCLR31_0 = 1 << LQ_GPIO_MAP[Pin][1];
        LQ_GPIO_Regs[LQ_GPIO_MAP[Pin][0]]->DOESET31_0  = 1 << LQ_GPIO_MAP[Pin][1];
    }
}


/********************************************************************************
 * @brief       GPIO_Pin初始化
 * 
 * @param       Pin_t		GPIO引脚
 * @param       Mode_t		GPIO模式
 * 
 * @example 配置：GPIO_Pin_A_15 为 推挽输出，无上下拉
 *          LQ_GPIO_Init(GPIO_Pin_A_15,GPIO_MODE_OUTPUT_PP,GPIO_RESISTOR_NO_PULL);
 ********************************************************************************/
void LQ_GPIO_Pin_Init(LQEnum_GPIO_Pin_t Pin_t,LQEnum_GPIO_Mode_t Mode_t,LQEnum_GPIO_Pull_t Pull_t)
{
   LQConfig_GPIO_InitTypeDef_t gpio_cfg = 
    {
 		.Mode  = Mode_t,	      // 配置输出模式
 		.Pull  = GPIO_RESISTOR_NO_PULL,	      // 配置浮空模式
 		.Speed = GPIO_SPEED_LOW	  // 配置低驱模式
    };
   LQ_GPIO_Init(Pin_t, &gpio_cfg);

}


/********************************************************************************
 * @brief       GPIO写入高低电平
 * 
 * @param       Pin			GPIO引脚
 * @param       Value		高低电平(非0及1)
 * 
 * @example     LQ_GPIO_WritePin(GPIO_Pin_A_28, 1);
 ********************************************************************************/
void LQ_GPIO_WritePin(LQEnum_GPIO_Pin_t Pin, uint8_t Value)
{
    if(Value == 0)
    {
        LQ_GPIO_Regs[LQ_GPIO_MAP[Pin][0]]->DOUTCLR31_0 = (1 << LQ_GPIO_MAP[Pin][1]);
    }
    else
    {
        LQ_GPIO_Regs[LQ_GPIO_MAP[Pin][0]]->DOUTSET31_0 = (1 << LQ_GPIO_MAP[Pin][1]);
    }
}

/********************************************************************************
 * @brief       GPIO电平翻转
 * 
 * @param       Pin			GPIO引脚
 * 
 * @example     LQ_GPIO_TogglePin(GPIO_Pin_A_28);
 ********************************************************************************/
void LQ_GPIO_TogglePin(LQEnum_GPIO_Pin_t Pin)
{
    LQ_GPIO_Regs[LQ_GPIO_MAP[Pin][0]]->DOUTTGL31_0 = (1 << LQ_GPIO_MAP[Pin][1]);
}

/********************************************************************************
 * @brief       读取GPIO高低电平
 * 
 * @param       Pin			GPIO引脚
 * 
 * @example     LQ_GPIO_ReadPin(GPIO_Pin_A_28);
 ********************************************************************************/
int LQ_GPIO_ReadPin(LQEnum_GPIO_Pin_t Pin)
{
    return ( LQ_GPIO_Regs[LQ_GPIO_MAP[Pin][0]]->DIN31_0 & (1 << LQ_GPIO_MAP[Pin][1]) ) ? 1 : 0;
}
