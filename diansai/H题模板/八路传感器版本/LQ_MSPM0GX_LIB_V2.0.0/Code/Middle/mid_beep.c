/*******************************************************************************
 * @file    mid_beep.c
 * @brief   蜂鸣器驱动实现（Middle Layer）
 *
 * @note    通过 LQ_GPIO_WritePin / LQ_GPIO_TogglePin 控制有源蜂鸣器的开关电平。
 *          有效电平由 MID_BEEP_ACTIVE_LEVEL 宏决定，避免硬编码 0/1，
 *          切换高/低电平触发的蜂鸣器模块时只需改头文件宏即可，无需改本文件。
 *
 *          关闭电平 MID_BEEP_OFF_LEVEL 在编译期由预处理器推导：
 *            - 有效电平=1 -> 关闭电平=0
 *            - 有效电平=0 -> 关闭电平=1
 *******************************************************************************/

#include "mid_beep.h"

/* ============================================================
 *  关闭电平推导（与有效电平相反）
 * ============================================================ */
#if (MID_BEEP_ACTIVE_LEVEL == 1)
    #define MID_BEEP_OFF_LEVEL     (0u)
#else
    #define MID_BEEP_OFF_LEVEL     (1u)
#endif

/*************************************************************************
 * @name     MID_Beep_Init
 *
 * @brief    初始化蜂鸣器控制引脚为推挽输出，并写入"关闭"电平
 * @param    none
 * @return   none
 *
 * @note     - 推挽输出：能直接驱动蜂鸣器模块的输入引脚
 *           - 无上下拉：蜂鸣器模块自身通常有确定电平，不需要 MCU 内部上下拉
 *           - 低速：蜂鸣器开关频率极低（人耳可闻的"滴滴"声最多几十 Hz），
 *             低速 IO 驱动能力已足够，且能降低 EMC 干扰
 *           - 初始化后默认关闭，避免上电瞬间误响
 *************************************************************************/
void MID_Beep_Init(void)
{
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
        .Mode  = GPIO_MODE_OUTPUT_PP,        /* 推挽输出 */
        .Pull  = GPIO_RESISTOR_NO_PULL,      /* 无上下拉 */
        .Speed = GPIO_SPEED_LOW,             /* 低速足够 */
    };
    LQ_GPIO_Init(MID_BEEP_PIN, &gpio_init);

    /* 初始化为关闭状态，防止上电误响 */
    LQ_GPIO_WritePin(MID_BEEP_PIN, MID_BEEP_OFF_LEVEL);
}

/*************************************************************************
 * @name     MID_Beep_On
 *
 * @brief    打开蜂鸣器（响）
 * @param    none
 * @return   none
 *
 * @note     向 MID_BEEP_PIN 写入有效电平 MID_BEEP_ACTIVE_LEVEL
 *************************************************************************/
void MID_Beep_On(void)
{
    LQ_GPIO_WritePin(MID_BEEP_PIN, MID_BEEP_ACTIVE_LEVEL);
}

/*************************************************************************
 * @name     MID_Beep_Off
 *
 * @brief    关闭蜂鸣器（停）
 * @param    none
 * @return   none
 *
 * @note     向 MID_BEEP_PIN 写入关闭电平 MID_BEEP_OFF_LEVEL
 *************************************************************************/
void MID_Beep_Off(void)
{
    LQ_GPIO_WritePin(MID_BEEP_PIN, MID_BEEP_OFF_LEVEL);
}

/*************************************************************************
 * @name     MID_Beep_Toggle
 *
 * @brief    翻转蜂鸣器状态（响<->停）
 * @param    none
 * @return   none
 *
 * @note     内部调用 LQ_GPIO_TogglePin，原子翻转。
 *           适合在主循环或定时中断里周期性调用，做"滴滴"报警声。
 *           例如：主循环每 200ms 调用一次，得到 2.5Hz 的"滴-滴-滴"报警声。
 *************************************************************************/
void MID_Beep_Toggle(void)
{
    LQ_GPIO_TogglePin(MID_BEEP_PIN);
}

/*************************************************************************
 * @name     MID_Beep_Beep
 *
 * @brief    让蜂鸣器连续响 count 次（每次响 on_seconds 秒，相邻两声间隔 gap_seconds 秒）
 * @param    count        响声次数，传 0 直接返回
 * @param    on_seconds   每次响的时长（秒），支持小数；<=0 直接返回
 * @param    gap_seconds  相邻两声之间的静默时长（秒），支持小数；<0 直接返回
 *                        传 0 表示两声紧贴（无间隔）
 * @return   none
 *
 * @note     阻塞式实现：
 *             on -> delay(on_ms) -> off -> [delay(gap_ms)] -> ...
 *           - count==0 / on_seconds<=0 / gap_seconds<0 时直接退出，不响不延时
 *           - 末次响声之后不再追加间隔，避免"响完多停一下"的拖尾感
 *           - 浮点秒 -> 整型毫秒向下取整，蜂鸣器对 1ms 误差无感
 *
 *           [注意] 会占用主循环，调用期间所有 PID / 编码器 / 通信都不更新，
 *             仅适合短促提示音（总时长 < 1~2s）。如需在主循环中非阻塞地
 *             驱动，请用 MID_Beep_On / MID_Beep_Off 自行实现状态机。
 *************************************************************************/
void MID_Beep_Beep(uint8_t count, float on_seconds, float gap_seconds)
{
    /* 非法入参直接退出 */
    if (count == 0u)         return;
    if (on_seconds <= 0.0f)  return;
    if (gap_seconds < 0.0f)  return;

    /* 浮点秒换算成整型毫秒（向下取整） */
    uint32_t on_ms  = (uint32_t)(on_seconds  * 1000.0f);
    uint32_t gap_ms = (uint32_t)(gap_seconds * 1000.0f);

    for (uint8_t i = 0; i < count; i++) {
        MID_Beep_On();
        delay_ms(on_ms);
        MID_Beep_Off();
        /* 最后一声后不再追加间隔 */
        if (i < (uint8_t)(count - 1u)) {
            delay_ms(gap_ms);
        }
    }
}
