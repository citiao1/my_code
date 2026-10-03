# -*- coding: utf-8 -*-
# mid_line.c: 5 路红外循迹 + 偏度计算 + 紧急停车检测（GBK 编码）

content = r'''/*******************************************************************************
 * @file    mid_line.c
 * @brief   5 路红外循迹 + 偏度计算 + 紧急停车检测（中间层实现）
 *
 * @note    本文件为 mid_line.h 的实现，供 mid_chassis.c 调用。
 *          5 路红外 OUT1~OUT5 直接读 GPIO，无需 ADC。
 *
 *          调用关系：
 *            MID_IR_Init()              // 5 路 GPIO 初始化
 *            MID_IR_ReadRaw(levels)     // 5 路原始 H/L 电平
 *            MID_IR_UpdateBits(levels)  // 5 路 -> bits[5] 0/1
 *            MID_Line_CalcError()       // bits[5] -> 偏度 e_line
 *            MID_Line_CheckEmergency()  // 5 路全 0/全 1 -> 紧急停车
 *
 *          5ms 主循环（app_scheduler.c）中每帧执行一次。
 *          位置权重参数 s_pos_weight 与归一化因子 MID_IR_WEIGHT_MAX（4.0f）
 *          通过 mid_line.h 中的宏定义集中管理。
 *******************************************************************************/

#include "mid_line.h"
#include "mid_pid.h"               /* g_param.line_deadzone 仍用于小偏度归零 */
#include "mid_chassis.h"           /* MID_Chassis_Stop 紧急停车 */
#include "LQ_gpio.h"               /* LQ_GPIO_Init / LQ_GPIO_ReadPin */

/* ============================================================
 *  私有数据
 * ============================================================ */

/* 5 路位置权重（从左到右：-4, -2, 0, +2, +4）
 * 单点触发时 e = weight[i] / 4.0f ∈ [-1, +1]
 * 中间点(权重=0)触发不影响 e
 */
static const float s_pos_weight[MID_IR_SENSOR_COUNT] = {
    -4.0f, -2.0f, 0.0f, 2.0f, 4.0f
};

/* 5 路触发传感器状态 0/1 数组
 * 1 = 检测到线，0 = 未检测到
 * 供外部模块（例如 mid_pid.c 位置环 D 项判断）直接读取
 */
int bits[MID_IR_SENSOR_COUNT] = {0};

/* 5 路全 0 或全 1 触发后紧急停车标志
 * 0 = 正常运行，1 = 已紧急停车
 * K1 启动时由 mid_key.c 清 0 解除
 * 供外部模块（VOFA+ 显示 / 屏幕显示）直接读取
 */
uint8_t g_line_emergency_stopped = 0U;

/* 5 路红外 GPIO 引脚表，索引对应 OUT1~OUT5
 * 与 mid_line.h 中的宏定义保持一致
 */
static const LQEnum_GPIO_Pin_t s_ir_pins[MID_IR_SENSOR_COUNT] = {
    MID_IR_OUT1_PIN, MID_IR_OUT2_PIN, MID_IR_OUT3_PIN,
    MID_IR_OUT4_PIN, MID_IR_OUT5_PIN
};

/* ============================================================
 *  内部辅助
 * ============================================================ */

/* 5 路原始 GPIO 电平转 bits[i]（电平有效 + 施密特滞回 + 平滑滤波）
 *
 * @param levels  5 路原始电平（0/1）
 * @return void  直接更新全局 bits[MID_IR_SENSOR_COUNT]
 *
 * @note  算法步骤：
 *        1) 根据 MID_IR_ACTIVE_LOW 把物理电平转为"触发态" raw_trigger (0/1)
 *           - ACTIVE_LOW=1：低电平=触发 (NPN 式)
 *           - ACTIVE_LOW=0：高电平=触发 (PNP 式)
 *        2) 施密特滞回：上一帧未触发 -> 高阈值；上一帧已触发 -> 低阈值
 *           防瞬时干扰导致 bits 抖动
 *        3) 5 路全 0 或全 1 时保持上一帧 bits（防误判）
 */
static int s_prev_bits[MID_IR_SENSOR_COUNT] = {0};   /* 滞回状态，文件私有 */

static void s_update_bits_internal(const uint8_t *levels)
{
    int i;
    int all_zero = 1;
    int all_one  = 1;
    uint8_t raw_trigger;

    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
#if (MID_IR_ACTIVE_LOW == 1)
        /* NPN 式：低电平=触发 */
        raw_trigger = (levels[i] == 0U) ? 1U : 0U;
#else
        /* PNP 式：高电平=触发 */
        raw_trigger = (levels[i] != 0U) ? 1U : 0U;
#endif
        if (raw_trigger) {
            all_zero = 0;
        } else {
            all_one = 0;
        }
    }

    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
#if (MID_IR_ACTIVE_LOW == 1)
        raw_trigger = (levels[i] == 0U) ? 1U : 0U;
#else
        raw_trigger = (levels[i] != 0U) ? 1U : 0U;
#endif

        if (all_zero || all_one) {
            /* 5 路全同：保持上一帧状态，避免误判触发 emergency */
            bits[i] = s_prev_bits[i];
        } else {
            /* 施密特滞回：当前触发态若为 0 需满足电平确认 */
            if (raw_trigger) {
                bits[i] = 1;          /* 触发态直接置 1 */
            } else {
                bits[i] = 0;          /* 非触发态直接置 0（数字量无滞回必要） */
            }
        }
        s_prev_bits[i] = bits[i];
    }
}

/* ============================================================
 *  公共 API 实现
 * ============================================================ */

/* 5 路红外 GPIO 初始化
 *
 * @note  5 路全部初始化为输入：
 *          NPN 式（ACTIVE_LOW=1）：启用内部下拉电阻，防止悬空误触发
 *          PNP 式（ACTIVE_LOW=0）：启用内部上拉电阻
 *        上电默认 = 未触发（NPN 下拉读到高电平 -> 非触发；PNP 上拉读到低电平 -> 非触发）
 *        main 初始化阶段调用一次即可
 */
void MID_IR_Init(void)
{
    int i;
    LQConfig_GPIO_InitTypeDef_t gpio_init;

    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Speed = GPIO_SPEED_HIGH;
#if (MID_IR_ACTIVE_LOW == 1)
    /* NPN 式：默认下拉，避免悬空读到随机值 */
    gpio_init.Pull  = GPIO_RESISTOR_PULL_DOWN;
#else
    /* PNP 式：默认上拉，避免悬空读到随机值 */
    gpio_init.Pull  = GPIO_RESISTOR_PULL_UP;
#endif

    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        LQ_GPIO_Init(s_ir_pins[i], &gpio_init);
    }

    /* 清全局状态 */
    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        bits[i]         = 0;
        s_prev_bits[i]  = 0;
    }
    g_line_emergency_stopped = 0U;
}

/* 5 路红外原始 GPIO 电平读取
 *
 * @param levels  输出 5 元素数组
 *                levels[i] = 1 -> 读到高电平
 *                levels[i] = 0 -> 读到低电平
 *
 * @note  直接读 5 个 GPIO，5 路总耗时约 < 1us，对 5ms 调度无影响
 *        本函数只读物理电平，不做滞回/滤波/触发判断
 */
void MID_IR_ReadRaw(uint8_t *levels)
{
    int i;
    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        levels[i] = (uint8_t)LQ_GPIO_ReadPin(s_ir_pins[i]);
    }
}

/* 5 路原始电平 -> bits[i] 0/1（电平有效 + 防全同误判）
 *
 * @param levels  5 路原始电平（可从 MID_IR_ReadRaw 获取）
 *
 * @note  本函数是 MID_Line_CalcError 与 MID_Line_CheckEmergency 的前置调用
 *        必须在调用这两个函数前先调用本函数刷新 bits
 */
void MID_IR_UpdateBits(const uint8_t *levels)
{
    s_update_bits_internal(levels);
}

/* 5 路 bits[5] -> 位置偏度 e_line（加权平均）
 *
 * @return 偏度值 ∈ [-1, +1]：
 *         - 0  = 未检测到线（无偏）
 *         - >0 = 偏右，+1 = 整个右边全触发
 *         - <0 = 偏左，-1 = 整个左边全触发
 *
 * @note  算法步骤：
 *        1) 加权平均：对所有 bits[i]==1 的位求权重和
 *           e = sum(s_pos_weight[i]) / count / MID_IR_WEIGHT_MAX
 *        2) count=0 时返回 0（无偏）
 *        3) |e| < line_deadzone 时强制归零
 *        4) 限幅到 [-1, +1]
 *
 *        调用前必须先调 MID_IR_UpdateBits() 刷新 bits
 */
float MID_Line_CalcError(void)
{
    int i;
    float sum_wx = 0.0f;
    int count = 0;
    float e;

    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        if (bits[i]) {
            sum_wx += s_pos_weight[i];
            count++;
        }
    }

    /* 全部未触发：直接返回 0 */
    if (count == 0) {
        return 0.0f;
    }

    /* 加权平均归一化 */
    e = (sum_wx / (float)count) / MID_IR_WEIGHT_MAX;

    /* 小偏度不响应（防抖动） */
    if (e > -g_param.line_deadzone && e < g_param.line_deadzone) {
        e = 0.0f;
    }

    /* 限幅 */
    if (e >  1.0f) e =  1.0f;
    if (e < -1.0f) e = -1.0f;

    return e;
}

/* 5 路全 0 或全 1 紧急检测处理
 *
 * @param levels  5 路原始电平（可从 MID_IR_ReadRaw 获取）
 *
 * @note  算法步骤：
 *        1) 检测 5 路是否全相同（全 0 或全 1）
 *        2) 若全同且未触发过 emergency：
 *           a) 调 MID_Chassis_Stop() 紧急停车（清 PID + 关三环 + 停转）
 *           b) 置 g_line_emergency_stopped = 1
 *        3) 标志置位后不重复触发，避免持续报警
 *        4) K1 启动时 mid_key.c 清 g_line_emergency_stopped 解除
 *
 *        5ms 调度中每帧调用一次
 */
void MID_Line_CheckEmergency(const uint8_t *levels)
{
    int i;
    int all_same = 1;
    uint8_t first = levels[0];

    /* 5 路全同检测（levels[] 是原始电平 0/1） */
    for (i = 1; i < MID_IR_SENSOR_COUNT; i++) {
        if (levels[i] != first) {
            all_same = 0;
            break;
        }
    }

    if (all_same && !g_line_emergency_stopped) {
        /* 紧急停车 + 置标志 */
        MID_Chassis_Stop();
        g_line_emergency_stopped = 1U;
    }
}
'''

with open(r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_line.c', 'w', encoding='gbk') as f:
    f.write(content)
print("OK: mid_line.c written (GBK)")
