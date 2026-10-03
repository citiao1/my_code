/*******************************************************************************
 * @file    mid_line.c
 * @brief   8 路红外循迹 + 偏度计算 + 紧急停车检测（中间层实现）
 *
 * @note    本文件为 mid_line.h 的实现，供 mid_chassis.c 调用。
 *          8 路红外 OUT1~OUT8 直接读 GPIO，无需 ADC。
 *
 *          调用关系：
 *            MID_IR_Init()              // 8 路 GPIO 初始化
 *            MID_IR_ReadRaw(levels)     // 8 路原始 H/L 电平
 *            MID_IR_UpdateBits(levels)  // 8 路 -> bits[8] 0/1
 *            MID_Line_CalcError()       // bits[8] -> 偏度 e_line
 *            MID_Line_CheckEmergency()  // 窗口内任意 4 路为 L 过 -> 紧急停车
 *            MID_Line_ResetEmergency()  // 清空紧急停止滑动窗口（K1 启动时调用）
 *
 *          5ms 主循环（app_scheduler.c）中每帧执行一次。
 *          位置权重参数 s_pos_weight 与归一化因子 MID_IR_WEIGHT_MAX（3.5f）
 *          通过 mid_line.h 中的宏定义集中管理。
 *******************************************************************************/

#include "mid_line.h"
#include "mid_chassis.h"           /* MID_Chassis_Stop 紧急停车 */
#include "LQ_gpio.h"               /* LQ_GPIO_Init / LQ_GPIO_ReadPin */

/* ============================================================
 *  私有数据
 * ============================================================ */

/* 8 路位置权重（从左到右，等间距：-3.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5, +3.5）
 * 单点触发时 e = weight[i] / 3.5f ∈ [-1, +1]
 * 中间两路(权重=-0.5/+0.5)触发时 e 很小，近似无偏
 */
static const float s_pos_weight[MID_IR_SENSOR_COUNT] = {
    -3.0f, -2.0f, -1.0f, -0.1f, 0.1f, 1.0f, 2.0f, 3.0f
};

/* 8 路触发传感器状态 0/1 数组
 * 1 = 检测到线，0 = 未检测到
 * 供外部模块（例如 mid_pid.c 位置环 D 项判断）直接读取
 */
int bits[MID_IR_SENSOR_COUNT] = {0};

/* 紧急停车标志：滑动窗口内 8 路中任意 4 路为 L 过时置 1
 * 0 = 正常运行，1 = 已紧急停车
 * K1 启动时由 mid_key.c 清 0 解除（同时调 MID_Line_ResetEmergency 清窗口）
 * 供外部模块（VOFA+ 显示 / 屏幕显示）直接读取
 */
uint8_t g_line_emergency_stopped = 0U;
/* 紧急停车使能开关：1=启用(K1模式/默认)，0=禁用(K2模式)
 * 由 mid_key.c 在 K1/K2 发车时设置，停止时恢复为 1
 */
uint8_t g_line_emergency_enable = 1U;

/* 8 路红外 GPIO 引脚表，索引对应 OUT1~OUT8
 * 与 mid_line.h 中的宏定义保持一致
 */
static const LQEnum_GPIO_Pin_t s_ir_pins[MID_IR_SENSOR_COUNT] = {
    MID_IR_OUT1_PIN, MID_IR_OUT2_PIN, MID_IR_OUT3_PIN,
    MID_IR_OUT4_PIN, MID_IR_OUT5_PIN, MID_IR_OUT6_PIN,
    MID_IR_OUT7_PIN, MID_IR_OUT8_PIN
};

/* 紧急停止滑动窗口大小：最近 N 次检测（N × 5ms）
 * 场景：半圆环入直道横线，车未回正，多个传感器依次扫过黑线
 * 默认 30 帧 = 150ms，需保证窗口期内多个传感器都能扫过黑线
 */
#define MID_LINE_EMG_WINDOW  (20U)

/* 紧急停车触发阈值：窗口内至少有 N 路检测到黑线（L）即触发
 * 默认 4 路 = 8 路中任意 4 路为 L 过即触发（比原 5 路全检测到更宽松）
 */
#define MID_LINE_EMG_MIN_SENSORS  (4U)

/* 紧急停止滑动窗口历史（环形缓冲区）
 * 每个元素是本次检测的 8 路 L 位图：bit i=1 表示第 i 路为 L
 */
static uint8_t s_emg_hist[MID_LINE_EMG_WINDOW] = {0};

/* 环形缓冲区写头位置：下一帧写入 s_emg_hist[s_emg_head] */
static uint16_t s_emg_head = 0U;

/* 窗口已积累的检测次数（0 ~ MID_LINE_EMG_WINDOW），未满时不判断 */
static uint16_t s_emg_hist_count = 0U;

/* ============================================================
 *  内部辅助
 * ============================================================ */

/* 8 路原始 GPIO 电平转 bits[i]（电平有效 + 施密特滞回 + 平滑滤波）
 *
 * @param levels  8 路原始电平（0/1）
 * @return void  直接更新全局 bits[MID_IR_SENSOR_COUNT]
 *
 * @note  算法步骤：
 *        1) 根据 MID_IR_ACTIVE_LOW 把物理电平转为"触发态" raw_trigger (0/1)
 *           - ACTIVE_LOW=1：低电平=触发 (NPN 式)
 *           - ACTIVE_LOW=0：高电平=触发 (PNP 式)
 *        2) 施密特滞回：上一帧未触发 -> 高阈值；上一帧已触发 -> 低阈值
 *           防瞬时干扰导致 bits 抖动
 *        3) 8 路全 0 或全 1 时保持上一帧 bits（防误判）
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
            /* 8 路全同：保持上一帧状态，避免误判触发 emergency */
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

/* 8 路红外 GPIO 初始化
 *
 * @note  8 路全部初始化为输入：
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
    g_line_emergency_enable = 1U;   /* 上电默认启用紧急停车检测（K1 模式行为） */

    /* 清紧急停止滑动窗口 */
    for (i = 0; i < MID_LINE_EMG_WINDOW; i++) {
        s_emg_hist[i] = 0U;
    }
    s_emg_head       = 0U;
    s_emg_hist_count = 0U;
}

/* 8 路红外原始 GPIO 电平读取
 *
 * @param levels  输出 8 元素数组
 *                levels[i] = 1 -> 读到高电平
 *                levels[i] = 0 -> 读到低电平
 *
 * @note  直接读 8 个 GPIO，8 路总耗时约 < 2us，对 5ms 调度无影响
 *        本函数只读物理电平，不做滞回/滤波/触发判断
 */
void MID_IR_ReadRaw(uint8_t *levels)
{
    int i;
    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        levels[i] = (uint8_t)LQ_GPIO_ReadPin(s_ir_pins[i]);
    }
}

/* 8 路原始电平 -> bits[i] 0/1（电平有效 + 防全同误判）
 *
 * @param levels  8 路原始电平（可从 MID_IR_ReadRaw 获取）
 *
 * @note  本函数是 MID_Line_CalcError 与 MID_Line_CheckEmergency 的前置调用
 *        必须在调用这两个函数前先调用本函数刷新 bits
 */
void MID_IR_UpdateBits(const uint8_t *levels)
{
    s_update_bits_internal(levels);
}

/* 8 路 bits[8] -> 位置偏度 e_line（加权平均）
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
 *        3) 限幅到 [-1, +1]
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

    /* 如果识别到中心传感器有值小偏度不响应（防抖动） */
//    if (bits[i]==0) {
//        e = 0.0f;
//    }

    /* 限幅 */
    if (e >  1.0f) e =  1.0f;
    if (e < -1.0f) e = -1.0f;

    return e;
}

/* 紧急停止检测：N 次滑动窗口内 8 路中任意 4 路为 L 过则触发
 *
 * @param levels  8 路原始电平（可从 MID_IR_ReadRaw 获取）
 *
 * @note  算法步骤：
 *        1) 计算本次 8 路 L 位图 cur_mask：levels[i]==0（低电平=黑线=L）
 *           则置 bit i
 *        2) 写入环形缓冲区 s_emg_hist[s_emg_head]，写头前进
 *           窗口未满 N 次时只积累不判断（避免上电/重启后误触发）
 *        3) 窗口满 N 次后，求窗口内全部 N 帧的 OR（or_mask）：
 *           or_mask 的 bit i=1 表示窗口内第 i 路至少为 L 过一次
 *           统计 or_mask 中 1 的个数（popcount），
 *           若 popcount >= MID_LINE_EMG_MIN_SENSORS（默认 4）
 *           表示窗口内 8 路中任意 4 路都为 L 过 -> 触发紧急停车
 *        4) 触发后调 MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
 *           标志置位后不重复触发
 *        5) K1 启动时 mid_key.c 调 MID_Line_ResetEmergency() 清窗口
 *
 *        N = MID_LINE_EMG_WINDOW（默认 30 帧 = 150ms）
 *        5ms 调度中每帧调用一次
 */
void MID_Line_CheckEmergency(const uint8_t *levels)
{
    /* K2 模式下禁用紧急停车检测（g_line_emergency_enable=0），直接返回不检测
     * K1 模式下 g_line_emergency_enable=1，执行现有检测逻辑
     */
    if (!g_line_emergency_enable) {
        return;
    }
    int i;
    uint8_t cur_mask = 0U;     /* 本次 8 路 L 位图，bit i=1 表示第 i 路为 L */
    uint8_t or_mask;
    uint8_t popcount;          /* or_mask 中 1 的个数 */
    uint8_t tmp;
    uint16_t idx;

    /* 1) 计算本次 8 路 L 位图：levels[i]==0 → L → 置 bit i */
    for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
        if (levels[i] == 0U) {
            cur_mask |= (uint8_t)(1U << i);
        }
    }

    /* 2) 写入环形缓冲区当前位置，写头前进 */
    s_emg_hist[s_emg_head] = cur_mask;
    s_emg_head = (uint16_t)((s_emg_head + 1U) % MID_LINE_EMG_WINDOW);
    if (s_emg_hist_count < MID_LINE_EMG_WINDOW) {
        s_emg_hist_count++;
    }

    /* 3) 窗口未满，不判断（避免上电/重启后误触发） */
    if (s_emg_hist_count < MID_LINE_EMG_WINDOW) {
        return;
    }

    /* 4) 求最近 N 帧（窗口内全部）的 OR，N = MID_LINE_EMG_WINDOW
     *    or_mask 的 bit i=1 表示窗口内第 i 路至少为 L 过一次 */
    or_mask = 0U;
    for (idx = 0U; idx < MID_LINE_EMG_WINDOW; idx++) {
        or_mask |= s_emg_hist[idx];
    }

    /* 5) 统计 or_mask 中 1 的个数（popcount）
     *    若 popcount >= MID_LINE_EMG_MIN_SENSORS（默认 4）
     *    表示窗口内 8 路中任意 4 路都为 L 过 -> 触发紧急停车 */
    popcount = 0U;
    tmp = or_mask;
    while (tmp) {
        popcount++;
        tmp &= (uint8_t)(tmp - 1U);   /* 清除最低位的 1 */
    }
    if (popcount >= MID_LINE_EMG_MIN_SENSORS) {
        if (!g_line_emergency_stopped) {
            /* 紧急停车 + 置标志 */
            MID_Chassis_Stop();
            g_line_emergency_stopped = 1U;
        }
    }
}

/* 清空紧急停止滑动窗口历史
 *
 * @note  仅清 s_emg_hist[] / s_emg_hist_count，不改 g_line_emergency_stopped
 *        （后者由 mid_key.c 单独清）
 *        K1 启动时调用，避免窗口残留导致重启后立即再次触发
 */
void MID_Line_ResetEmergency(void)
{
    int i;
    for (i = 0; i < MID_LINE_EMG_WINDOW; i++) {
        s_emg_hist[i] = 0U;
    }
    s_emg_head      = 0U;
    s_emg_hist_count = 0U;
}
