/*******************************************************************************
 * @file                LQ_key.c
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
#include "LQ_key.h"

/*************************************************************************
 * @name     LQ_Key_Init
 *
 * @brief    用户调用 按键和拨码开关初始化程序
 * @param    none
 * @return   none
 *
 * @note     按键和拨码开关初始化程序
 *************************************************************************/
void LQ_Key_Init(void)
{
    LQConfig_GPIO_InitTypeDef_t gpio_input_init = {
        .Mode  = GPIO_MODE_OUTPUT_PP,       // 设置推挽输出模式
        .Pull  = GPIO_RESISTOR_PULL_UP,     // 设置上拉电阻
        .Speed = GPIO_SPEED_LOW,            // 设置 GPIO 速度为低速模式 
    };

    LQ_GPIO_Init(KEY_0P, &gpio_input_init); // 按键初始化0
    LQ_GPIO_Init(KEY_1P, &gpio_input_init); // 按键初始化1
    LQ_GPIO_Init(KEY_2P, &gpio_input_init); // 按键初始化2

    LQ_GPIO_Init(DSW_0P, &gpio_input_init); // 拨码开关初始化0
    LQ_GPIO_Init(DSW_1P, &gpio_input_init); // 拨码开关初始化1
}

/*************************************************************************
 * @name     LQ_Key_Read
 *
 * @brief    读取按键或拨码开关的原始状态
 * @param    keyno ：按键编号， KEY0/KEY1/KEY2/DSW0/DSW1
 * @return   按键状态，0按下/1松开
 *
 * @note     读取按键或拨码开关的原始状态
 *************************************************************************/
uint8_t LQ_Key_Read(LQEnum_KEYn_t keyno)
{
    switch(keyno)
    {
        case KEY0:  return (uint8_t)LQ_GPIO_ReadPin(KEY_0P);
        case KEY1:  return (uint8_t)LQ_GPIO_ReadPin(KEY_1P);
        case KEY2:  return (uint8_t)LQ_GPIO_ReadPin(KEY_2P);
        case DSW0:  return (uint8_t)LQ_GPIO_ReadPin(DSW_0P);
        case DSW1:  return (uint8_t)LQ_GPIO_ReadPin(DSW_1P);
        default:    return RELEASED;
    }
}

/*************************************************************************
 * @name     LQ_Key_Read_All
 *
 * @brief    读取所有按键的状态，返回组合值
 * @param    none
 * @return   按键组合状态，bit0-KEY0, bit1-KEY1, bit2-KEY2
 *
 * @note     只返回按键状态，拨码开关不包括
 *************************************************************************/
uint8_t LQ_Key_Read_All(void)
{
    uint8_t state = 0;
    state |= (LQ_Key_Read(KEY0) == PRESSED) ? 0x01 : 0x00;
    state |= (LQ_Key_Read(KEY1) == PRESSED) ? 0x02 : 0x00;
    state |= (LQ_Key_Read(KEY2) == PRESSED) ? 0x04 : 0x00;
    return state;
}

/*************************************************************************
 * @name     LQ_Key_Scan
 *
 * @brief    带去抖动的按键扫描
 * @param    keyno ：按键编号
 * @return   按键事件，0无事件/1按下/2松开
 *
 * @note     需要在定时器中调用此函数，间隔10-20ms
 *************************************************************************/
uint8_t LQ_Key_Scan(LQEnum_KEYn_t keyno)
{
    uint8_t current = LQ_Key_Read(keyno);
    uint8_t event = 0;

    if (current != key_last[keyno])
    {
        key_time[keyno] = 0; // 重置计数
    }
    else
    {
        if (key_time[keyno] < 5) // 去抖时间，假设10ms调用一次，5*10=50ms
        {
            key_time[keyno]++;
        }
        else
        {
            if (key_state[keyno] != current)
            {
                key_state[keyno] = current;
                if (current == PRESSED)
                {
                    event = 1; // 按下事件
                }
                else
                {
                    event = 2; // 松开事件
                }
            }
        }
    }
    key_last[keyno] = current;
    return event;
}
