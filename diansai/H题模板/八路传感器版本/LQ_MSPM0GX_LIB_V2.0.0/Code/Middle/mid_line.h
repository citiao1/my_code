#ifndef __MID_LINE_H__
#define __MID_LINE_H__

/*******************************************************************************
 * @file    mid_line.h
 * @brief   8路红外循迹中间层（接口+偏度计算）
 *
 * @note    本文件提供 8 路红外传感器 GPIO 直接读取 + 位置偏度映射。
 *          由 mid_chassis.c 调用获取当前偏度值 e_line 并转换为控制量。
 *          实现在 mid_line.c。
 *
 *          8 路红外传感器硬件连接（OUT1~OUT8 接 MCU 的 GPIO）：
 *              OUT1 -> PA27
 *              OUT2 -> PA26
 *              OUT3 -> PA25
 *              OUT4 -> PA24
 *              OUT5 -> PB25
 *              OUT6 -> PB24
 *              OUT7 -> PB20
 *              OUT8 -> PA22
 *
 *          有效电平由 MID_IR_ACTIVE_LOW 控制：
 *              1 = NPN 式（检测到线=低电平有效，推荐/默认）
 *              0 = PNP 式（检测到线=高电平有效）
 *
 *          调用时序（app_scheduler 每 5ms 执行一次）：
 *              MID_IR_Init();                     // 8 路 GPIO 初始化
 *              MID_IR_ReadRaw(levels);            // 8 路原始 H/L 电平
 *              MID_IR_UpdateBits(levels);         // 8 路 -> bits[i] 0/1 状态
 *              float e = MID_Line_CalcError();    // bits[8] -> 偏度 e_line
 *              MID_Line_CheckEmergency(levels);   // N 次窗口内任意 4 路为 L 过 -> 紧急停车
 *              MID_Line_ResetEmergency();         // 清空紧急停止滑动窗口（K1 启动时调用）
 *
 *          bits[i] 定义：1 = 检测到线，0 = 未检测到
 *          e_line 定义：>0 偏右，<0 偏左，范围 [-1, +1]，内部已限幅
 *
 *          8 路位置权重（从左到右，等间距）：
 *                 0     1     2     3     4     5     6     7
 *               -3.5 -2.5  -1.5  -0.5  +0.5  +1.5  +2.5  +3.5
 *          e = (sum s_pos_weight[i] where bits[i]==1) / count / 3.5f
 *          全未触发 -> 返回 e=0；全触发 -> e=sign(权重和)
 *
 *          紧急停车触发条件：
 *              最近 N 次检测的滑动窗口内，8 路中任意 4 路（或更多）
 *              都至少为 L 过一次（小车扫过横线/路口时多路依次压线的场景）
 *******************************************************************************/

#include "include.h"

/* ============================================================
 *  硬件配置
 * ============================================================ */

/* 8 路红外传感器 GPIO 引脚表，OUT1~OUT8 对应 8 路循迹传感器
 * 注：PA22 原为 WDD35D4 角度传感器 ADC 引脚，现已改作 OUT8 数字输入
 */
#define MID_IR_OUT1_PIN     GPIO_Pin_A_27        /* 左外侧 OUT1 */
#define MID_IR_OUT2_PIN     GPIO_Pin_A_26        /* 左内侧 OUT2 */
#define MID_IR_OUT3_PIN     GPIO_Pin_A_25        /* 中间   OUT3 */
#define MID_IR_OUT4_PIN     GPIO_Pin_A_24        /* 右内侧 OUT4 */
#define MID_IR_OUT5_PIN     GPIO_Pin_B_25        /* 右外侧 OUT5 */
#define MID_IR_OUT6_PIN     GPIO_Pin_B_24        /* 新增   OUT6 */
#define MID_IR_OUT7_PIN     GPIO_Pin_B_20        /* 新增   OUT7 */
#define MID_IR_OUT8_PIN     GPIO_Pin_A_22        /* 新增   OUT8（原角度传感器引脚） */

/* 8 路红外传感器有效电平
 * 1 = NPN 式（检测到线=低电平=1，推荐/默认）
 * 0 = PNP 式（检测到线=高电平=1）
 * 按实际传感器型号，修改本宏即可
 */
#define MID_IR_ACTIVE_LOW   (1)

/* 8 路位置权重归一化因子，除以 3.5f 得到 -1 ~ +1 */
#define MID_IR_WEIGHT_MAX   (3.5f)

/* 8 路红外传感器总数 */
#define MID_IR_SENSOR_COUNT (8)

/* ============================================================
 *  全局状态
 * ============================================================ */

/* 8 路触发传感器状态 0/1 数组（mid_line.c 内定义）
 * 1 = 检测到线，0 = 未检测到
 * 供外部模块（例如 mid_pid.c 位置环 D 项判断）直接读取
 */
extern int bits[MID_IR_SENSOR_COUNT];

/* 紧急停车标志：N 次滑动窗口内 8 路中任意 4 路为 L 过时置 1
 * 0 = 正常运行，1 = 已紧急停车
 * K1 启动时由 mid_key.c 清 0 解除（同时调 MID_Line_ResetEmergency 清窗口）
 * 供外部模块（VOFA+ 显示 / 屏幕显示）直接读取
 */
extern uint8_t g_line_emergency_stopped;
/* 紧急停车使能开关（由 mid_key.c 在发车时设置）
 * 1 = 启用紧急停车检测（K1 模式，默认）
 * 0 = 禁用紧急停车检测（K2 模式，脱线不停车）
 * 上电默认 = 1（兼容现有行为）
 */
extern uint8_t g_line_emergency_enable;

/* ============================================================
 *  API 函数声明
 * ============================================================ */

/* 8 路红外 GPIO 初始化
 * @note  在 main 初始化阶段调用一次，初始化为输入（NPN 式默认内部下拉电阻）
 *        若实际传感器为 PNP 式，需将 MID_IR_ACTIVE_LOW 改为 0
 */
void MID_IR_Init(void);

/* 8 路红外原始电平 0/1 读取（读原始 GPIO 电平）
 * @param levels  输出 8 元素 uint8_t 数组，levels[i]=0 表示低电平
 * @note  1 = 读到高电平，0 = 读到低电平
 *        供 main.c 屏幕刷新显示 H/L
 */
void MID_IR_ReadRaw(uint8_t *levels);

/* 8 路红外 bits 0/1 状态更新（电平+滞回+平滑滤波）
 * @param levels  8 路原始电平（可从 MID_IR_ReadRaw 获取）
 * @note  施密特滞回防瞬时干扰，8 路全 0 或全 1 时保持上一帧 bits
 */
void MID_IR_UpdateBits(const uint8_t *levels);

/* 8 路 bits[8] -> 位置偏度 e_line，范围 -1 ~ +1
 * @return 偏度值：
 *         - 0  = 未检测到线（无偏）
 *         - >0 = 偏右，+1 = 整个右边全触发
 *         - <0 = 偏左，-1 = 整个左边全触发
 * @note  8 路全 0 时 e=0，8 路全 1 时 e=sign(权重和)
 *        调用前必须先调 MID_IR_UpdateBits() 刷新 bits
 */
float MID_Line_CalcError(void);

/* 紧急停止检测：N 次滑动窗口内 8 路中任意 4 路为 L 过则触发
 * @param levels  8 路原始电平（可从 MID_IR_ReadRaw 获取）
 * @note  最近 N 次检测窗口内（N = MID_LINE_EMG_WINDOW，默认 30 帧 = 150ms），
 *        8 路 bit 位图 OR 后统计其中 1 的个数（popcount），
 *        若 popcount >= MID_LINE_EMG_MIN_SENSORS（默认 4）则触发：
 *        调 MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
 *        窗口未满 N 次时不判断（避免上电/重启后误触发）
 *        K1 启动时 mid_key.c 调 MID_Line_ResetEmergency() 清窗口
 *        5ms 调度中每帧调用 1 次（app_scheduler.c）
 *        一次 emergency 触发后标志置位，不重复触发
 */
void MID_Line_CheckEmergency(const uint8_t *levels);

/* 清空紧急停止滑动窗口历史
 * @note  仅清 s_emg_hist[] / s_emg_hist_count，不改 g_line_emergency_stopped
 *        （后者由 mid_key.c 单独清）
 *        K1 启动时调用，避免窗口残留导致重启后立即再次触发
 */
void MID_Line_ResetEmergency(void);


#endif /* __MID_LINE_H__ */
