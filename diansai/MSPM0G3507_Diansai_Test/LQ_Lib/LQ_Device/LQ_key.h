/*******************************************************************************
 * @file                LQ_key.h
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
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_KEY_H__
#define __LQ_KEY_H__

#include "include.h"

//------------------------- 宏定义 KEY 管脚 -------------------------
// 按键管脚定义
#define KEY_0P              GPIO_Pin_B_14       // 母板上按键0
#define KEY_1P              GPIO_Pin_B_15       // 母板上按键1
#define KEY_2P              GPIO_Pin_B_16       // 母板上按键2

// 拨码开关管脚定义
#define DSW_0P              GPIO_Pin_B_6        // 拨码开关0
#define DSW_1P              GPIO_Pin_B_8        // 拨码开关1

// 枚举按键编号
typedef enum
{
    KEY0 = 0,
    KEY1 = 1,
    KEY2 = 2,
    DSW0 = 3,
    DSW1 = 4,
}LQEnum_KEYn_t;

// 按键状态枚举
typedef enum
{
    PRESSED  = 0,  // 按下状态
    RELEASED = 1,  // 松开状态
}LQEnum_KEY_State_t;

// 内部变量，用于去抖动
static uint8_t  key_state[5] = {RELEASED, RELEASED, RELEASED, RELEASED, RELEASED};
static uint8_t  key_last [5] = {RELEASED, RELEASED, RELEASED, RELEASED, RELEASED};
static uint16_t key_time [5] = {0, 0, 0, 0, 0};

void LQ_Key_Init(void);                     /*! @brief      按键初始化 */

uint8_t LQ_Key_Read(LQEnum_KEYn_t keyno);   /*! @brief      单个按键读取(包括拨码开关) */
uint8_t LQ_Key_Read_All(void);              /*! @brief      读取所有按键(不包括拨码开关) */
uint8_t LQ_Key_Scan(LQEnum_KEYn_t keyno);   /*! @brief      按键扫描 */

#endif
