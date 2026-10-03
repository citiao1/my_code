#ifndef __MID_OLED_H__
#define __MID_OLED_H__

/*******************************************************************************
 * @file    mid_oled.h
 * @brief   0.96 寸 OLED 显示中间层（Middle Layer）
 *
 * @note    本模块**完全独立于 LQ_oled.c**，不依赖其任何代码。
 *          重新定位实现以下能力：
 *          - 128×64 单色 OLED 软件 SPI 驱动（位翻转 GPIO）
 *          - 内部 1024 字节 GRAM 缓存（s_oled_gram[128][8]）
 *          - 6×8 / 6×12 ASCII 字符显示（使用 LQ_font.c 的 asc2_0806/1206 字模）
 *          - **分片刷屏** -- 把 1024 字节拆 8 页（每页 128 字节），
 *            每次 MID_OLED_Task() 只刷 1 页，把单次阻塞从 ~2ms 降到 ~0.3ms
 *          - **脏标志** -- 屏幕无变化时 Task 0 开销
 *
 *          设计原则：
 *          - 屏幕代码**只在 main loop** 跑，不进 5ms 调度器
 *          - 渲染（ShowString/ShowNum/Clear）只改 GRAM，不碰 GPIO
 *          - 真正的 SPI 发送由 MID_OLED_Task() 分片异步完成
 *          - 5ms 调度永不被打乱：main loop 优先级
 *            APP_Scheduler_Run() -> APP_DEBUG_PollPlot() -> MID_Key_Scan() ->
 *            MID_OLED_Task() -> __WFI
 *
 *          引脚：
 *            SCL=PA17  SDA=PA16  CS=PB22  RES=PB21  DC=PB23
 *          （与电机/编码器/IMU/UART/灰度/蜂鸣器均无冲突）
 *
 * @author  LQ_012
 * @date    2026-07-27
 *******************************************************************************/

#include "include.h"

/* ============================================================
 *  硬件配置
 * ============================================================ */

/* OLED 5 个 GPIO 定义（LQ 板上） */
#define MID_OLED_SCL_PIN     (GPIO_Pin_A_17)   /* SPI 时钟 */
#define MID_OLED_SDA_PIN     (GPIO_Pin_A_16)   /* SPI 数据 */
#define MID_OLED_CS_PIN      (GPIO_Pin_B_22)   /* 片选 */
#define MID_OLED_RES_PIN     (GPIO_Pin_B_21)   /* 复位（低复位） */
#define MID_OLED_DC_PIN      (GPIO_Pin_B_23)   /* 数据/命令选择（0=命令，1=数据） */

/* OLED 分辨率：宽 128 像素，高 64 像素，分 8 页（每页 8 行） */
#define MID_OLED_W           (128u)
#define MID_OLED_PAGES       (8u)
#define MID_OLED_PAGE_BYTES  (128u)

/* 字体大小 */
#define MID_OLED_FONT_8      (8u)              /* 6×8 字模，每字符 6 像素宽 */
#define MID_OLED_FONT_12     (12u)             /* 6×12 字模，每字符 6 像素宽 */

/* ============================================================
 *  API
 * ============================================================ */

/* 初始化 OLED（5 个 GPIO 推挽输出 + 复位 + 发送初始化命令序列 + 清 GRAM）
 * @note  必须在使用其他 API 之前调用一次
 *        重复调用无副作用，仅 GPIO 重新初始化
 *        耗时约 100ms，期间 main loop 未启动
 */
void MID_OLED_Init(void);

/* main loop 分片刷屏任务
 * @note  **必须在 main loop 调用**，不能进 5ms 调度器 / 中断
 *        行为：
 *          - 每次只刷 1 页 128 字节（实测阻塞约 0.15 ~ 0.5ms）
 *          - dirty=0 时 0 开销
 *          - 8 页全刷需 main loop 走 8 次
 *            5ms tick 频率下，共 ~40ms = 25Hz 刷新率（OLED 观察无抖动）
 */
void MID_OLED_Task(void);

/* 清空 GRAM 内容（仅清 GRAM 内存，dirty=1 + page=0 从头重刷）
 * @note  渲染层，只改 GRAM + 置脏标志
 *        屏幕实际清空要等 MID_OLED_Task() 把 8 页全刷完
 */
void MID_OLED_Clear(void);

/* 在指定位置显示 ASCII 字符串
 * @param  y     行号 0~7（6×8 字模占 1 行；6×12 字模占 2 行）
 * @param  x     起始列 0~127
 * @param  str   ASCII 字符串指针（含 null）
 * @param  size  字体大小 MID_OLED_FONT_8 或 MID_OLED_FONT_12
 * @note  不可见 ASCII 字符（<32 或 >126）被截断
 *        调用此函数 GRAM 变化，dirty=1
 */
void MID_OLED_ShowString(uint8_t y, uint8_t x, const char *str, uint8_t size);

/* 在指定位置显示无符号整数（自动补 0 到 len 位）
 * @param  x     起始列 0~127
 * @param  y     起始行 0~63（像素单位，不是行号）
 * @param  num   要显示的值
 * @param  len   显示位数（超过 num 实际位时高位补 0）
 * @param  size  字体大小 MID_OLED_FONT_8 或 MID_OLED_FONT_12
 * @param  mode  1=白字黑底，0=黑字白底
 * @note  调用此函数 GRAM 变化，dirty=1
 */
void MID_OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len,
                      uint8_t size, uint8_t mode);

/* 在指定位置显示浮点数（带符号）
 * @param  y     行号 0~7
 * @param  x     起始列 0~127
 * @param  val   浮点数
 * @param  prec 小数位数（0~4）
 * @param  size  字体大小 MID_OLED_FONT_8 或 MID_OLED_FONT_12
 * @note  使用 snprintf 格式化为 "-12.34"，内部占 16 字节栈
 *        调用此函数 GRAM 变化，dirty=1
 */
void MID_OLED_ShowFloat(uint8_t y, uint8_t x, float val, uint8_t prec, uint8_t size);

#endif
