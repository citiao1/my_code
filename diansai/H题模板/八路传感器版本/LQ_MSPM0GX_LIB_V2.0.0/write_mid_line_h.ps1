$content = @'
#ifndef __MID_LINE_H__
#define __MID_LINE_H__

/*******************************************************************************
 * @file    mid_line.h
 * @brief   5·红外循迹中坚层（驱动+位置偏度计算）
 *
 * @note    本文件提供 5·红外传感器的 GPIO 直接读取 + 位置偏度映射。
 *          供 mid_chassis.c 调用获取当前偏度值 e_line 进行转向控制。
 *          实现见 mid_line.c。
 *
 *          5·红外传感器硬件配置（OUT1~OUT5 与 MCU 的 GPIO 连接）：
 *              OUT1 -> PA27
 *              OUT2 -> PA26
 *              OUT3 -> PA25
 *              OUT4 -> PA24
 *              OUT5 -> PB25
 *
 *          有效电平（由 MID_IR_ACTIVE_LOW 控制）：
 *              1 = NPN 式（检测到线=低电平有效）（推荐/默认）
 *              0 = PNP 式（检测到线=高电平有效）
 *
 *          调用时序（app_scheduler 每 5ms 执行一次）：
 *              MID_IR_Init();                     // 5· GPIO 初始化
 *              MID_IR_ReadRaw(levels);            // 5· 原始 H/L 电平
 *              MID_IR_UpdateBits(levels);         // 5· 更新 bits[i] 0/1 状态
 *              float e = MID_Line_CalcError();    // bits[5] -> 偏度 e_line
 *              MID_Line_CheckEmergency(levels);   // 5·全 0 或全 1 触发停车
 *
 *          bits[i] 含义：1 = 检测到线，0 = 未检测到
 *          e_line 含义：>0 偏右，<0 偏左，范围 [-1, +1]（内部限幅）
 *
 *          5·位置权重（从左到右）：
 *                 0     1     2     3     4
 *                -4    -2     0    +2    +4
 *          e = (sum s_pos_weight[i] where bits[i]==1) / count / 4.0f
 *          全未触发 -> 返回 e=0；全触发 -> e=sign(权重和)
 *
 *          5·全 0 或全 1 -> 检测不到信号或丢失线 -> 触发停车
 *******************************************************************************/

#include "include.h"

/* ============================================================
 *  硬件配置
 * ============================================================ */

/* 5·红外传感器 GPIO 引脚（OUT1~OUT5 对应 5·循迹）
 * 注意：PA22 被 WDD35D4 角度传感器占用，不在内
 */
#define MID_IR_OUT1_PIN     GPIO_Pin_A_27        /* 红外 OUT1 */
#define MID_IR_OUT2_PIN     GPIO_Pin_A_26        /* 红外 OUT2 */
#define MID_IR_OUT3_PIN     GPIO_Pin_A_25        /* 红外 OUT3 */
#define MID_IR_OUT4_PIN     GPIO_Pin_A_24        /* 红外 OUT4 */
#define MID_IR_OUT5_PIN     GPIO_Pin_B_25        /* 红外 OUT5 */

/* 5·红外传感器有效电平
 * 1 = NPN 式（检测到线=低电平=1）（推荐/默认）
 * 0 = PNP 式（检测到线=高电平=1）
 * 如实际传感器反相，修改本宏重编即可
 */
#define MID_IR_ACTIVE_LOW   (1)

/* 5·位置权重（最大 4.0f 归一化到 -1 ~ +1） */
#define MID_IR_WEIGHT_MAX   (4.0f)

/* 5·红外传感器总数 */
#define MID_IR_SENSOR_COUNT (5)

/* ============================================================
 *  全局状态
 * ============================================================ */

/* 5·触发传感器状态 0/1 数组（mid_line.c 内定义）
 * 1 = 检测到线，0 = 未检测到
 * 供外部模块读取（例如位置环 D 项判断）
 */
extern int bits[MID_IR_SENSOR_COUNT];

/* 5·全 0 或全 1 触发后紧急停车标志
 * 0 = 正常运行，1 = 已紧急停车
 * K1 启动时 mid_key.c 清 0 解除
 * 供外部模块（VOFA+ 显示 / 屏幕显示）直接读取
 */
extern uint8_t g_line_emergency_stopped;

/* ============================================================
 *  API 函数声明
 * ============================================================ */

/* 5·红外 GPIO 初始化
 * @note  在 main 初始化中调用一次，初始化为输入（NPN 式默认内部下拉）
 *        如实际传感器 PNP 式，需将 MID_IR_ACTIVE_LOW 改为 0
 */
void MID_IR_Init(void);

/* 5·红外原始电平 0/1 读取（原始 GPIO 电平）
 * @param levels  输出 5 元素 uint8_t 数组，levels[i]=0 表示低电平
 * @note  1 = 读到高电平，0 = 读到低电平
 *        供 main.c 屏幕刷新显示 H/L
 */
void MID_IR_ReadRaw(uint8_t *levels);

/* 5·更新 bits 0/1 状态（电平+滞回+平滑滤波）
 * @param levels  5·原始电平（可从 MID_IR_ReadRaw 获取）
 * @note  施密特触发防抖：低阈值 1/3，高阈值 2/3，抑制瞬时干扰
 *        5·全 0 或全 1 时保持 bits 不变
 */
void MID_IR_UpdateBits(const uint8_t *levels);

/* 5·bits[5] -> 位置偏度 e_line，范围 -1 ~ +1
 * @return 偏度值：
 *         - 0  = 未检测到线（无偏）
 *         - >0 = 偏右，+1 = 整个右边全触发
 *         - <0 = 偏左，-1 = 整个左边全触发
 * @note  5·全 0 时 e=0；5·全 1 时 e=sign(权重和)
 *        |e| < line_deadzone 时强制归零（小偏度不响应）
 *        调用本函数前需先调用 MID_IR_UpdateBits() 刷新 bits
 */
float MID_Line_CalcError(void);

/* 5·全 0 或全 1 紧急检测处理
 * @param levels  5·原始电平（可从 MID_IR_ReadRaw 获取）
 * @note  5·全 0 或全 1 时 -> 调 MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
 *        K1 启动时 mid_key.c 清 g_line_emergency_stopped
 *        5ms 调度中调用 1 次（app_scheduler.c）
 *        一次 emergency 触发后，未清标志前 不会重复触发
 */
void MID_Line_CheckEmergency(const uint8_t *levels);

#endif /* __MID_LINE_H__ */
'@

[System.IO.File]::WriteAllText('Code\Middle\mid_line.h', $content, [System.Text.Encoding]::Default)
Write-Host "OK: mid_line.h written"
