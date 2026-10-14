/*******************************************************************************
 * @file    main.c
 * @brief   循迹小车 - 竞赛型 PID 控制器
 *
 * @note    程序流程：
 *          1. 系统初始化（80MHz 主频）
 *          2. 初始化 PID 参数（默认急停，防止上电飞车）
 *          3. 初始化调试串口（UART0 @ 9600 波特率，VOFA+ 通信）
 *          4. 初始化电动 PWM（10kHz）
 *          5. 初始化编码器
 *          6. 初始化 8 路红外传感器（数字量 GPIO 模式读取）
 *              OUT1 -> PA27, OUT2 -> PA26, OUT3 -> PA25, OUT4 -> PA24,
 *              OUT5 -> PB25, OUT6 -> PB24, OUT7 -> PB20, OUT8 -> PA22
 *          7. LSM6DSV16X IMU 初始化（SPI 通信）
 *          8. 初始化电动 PID 状态
 *          9. 初始化调度器（SysTick 1ms 定时器中断）
 *         10. 初始化按键 + OLED + K1 启动
 *         11. 蜂鸣器初始化 + 上电提示音
 *         12. 主循环：渲染 + OLED 刷新 + 调度器 + 低功耗
 *
 *          8 路 IR 传感器启动流程：
 *            - 8 路 GPIO 数字量读取
 *            - 8 路全 H/L 图显到 OLED 屏
 *            - 8 路中任意 4 路检测到黑线触发紧急停车
 *            - STATUS: RUN / STOP / E-STOP 状态显示
 *
 *          K1 按键控制启动/急停，并重置 PID
 *
 * @author  LQ_012
 * @date    2026-07-29
 *******************************************************************************/

#include "include.h"
#include "LQ_device.h"
#include "LQ_lsm6dsv16x.h"
/* LQ_tracking.h 已弃用：8 路红外由 mid_line 模块直接 GPIO 读取 */
#include "mid_motor.h"
#include "mid_encoder.h"
#include "mid_chassis.h"
#include "mid_pid.h"
#include "mid_line.h"            /* 8 路红外传感器 + 紧急停车 */
#include "mid_oled.h"            /* 0.96 寸 OLED 显示 */
#include "mid_key.h"             /* K1 启动/急停 + 20ms 扫描 */
#include "mid_imu.h"
#include "mid_beep.h"
#include "app_debug.h"
#include "app_scheduler.h"

/* ============================================================
 *  OLED 屏幕刷新配置
 * ============================================================ */

/* 屏幕刷新周期：每 50ms 刷新一次，避免对 GRAM 频繁操作 */
#define SCREEN_REFRESH_MS         (50u)

/* 上次屏幕刷新的 1ms 时间戳 */
static uint32_t s_last_screen_ms = 0u;

/* ============================================================
 *  8 路 IR 传感器辅助
 * ============================================================ */

/* 原显示辅助函数（H/L 字符格式化），原 OLED 显示改运行时间后不再使用
 * 保留代码不删除，注释掉以消除"未使用函数"警告
 * static void format_levels(const uint8_t *levels, char *states)
 * {
 *     int i;
 *     for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
 *         states[i] = levels[i] ? 'H' : 'L';
 *     }
 *     states[MID_IR_SENSOR_COUNT] = '\0';
 * }
 */

/* ============================================================
 *  OLED 屏幕刷新（运行时间显示）
 *
 *  原功能：显示 8 路 H/L 字符 + 状态标签
 *  现功能：仅显示运行时间（秒）
 *
 *  刷新周期：50ms
 *    渲染层只改 GRAM，不碰 GPIO
 *    真正的 SPI 发送由 MID_OLED_Task() 分片异步完成
 *    渲染后置脏标志，Task 自动刷屏
 */
static void s_screen_refresh(void)
{
    uint32_t now = APP_Scheduler_GetTickMs();
    if ((now - s_last_screen_ms) < SCREEN_REFRESH_MS) {
        return;
    }
    s_last_screen_ms = now;

    /* ===== 原 OLED 显示已注释（保留代码不删除） =====
     * 原显示内容：STATUS 状态行 + IR1~IR8 标签 + 8 路 H/L 字符
     * 改为显示运行时间后不再需要，注释保留
     *
     * // 1) 读 8 路原始电平，直接读 GPIO
     * uint8_t levels[MID_IR_SENSOR_COUNT];
     * MID_IR_ReadRaw(levels);
     *
     * // 2) H/L 字符格式化
     * char states[9];
     * format_levels(levels, states);
     *
     * // 3) 显示状态标签
     * const char *status_str;
     * if (g_line_emergency_stopped) {
     *     status_str = "STATUS:E-STOP";
     * } else if (MID_Key_IsRunning()) {
     *     status_str = "STATUS:  RUN";
     * } else {
     *     status_str = "STATUS: STOP";
     * }
     * MID_OLED_ShowString(0u, 0u, status_str, MID_OLED_FONT_8);
     *
     * // 4) 显示 8 路标签
     * MID_OLED_ShowString(1u, 0u, "IR1 IR2 IR3 IR4 IR5 IR6 IR7 IR8", MID_OLED_FONT_8);
     *
     * // 5) 显示 8 路 H/L
     * {
     *     uint8_t x_pos = 6u;
     *     int i;
     *     for (i = 0; i < MID_IR_SENSOR_COUNT; i++) {
     *         char buf[2] = {states[i], '\0'};
     *         MID_OLED_ShowString((uint8_t)(2u + i), x_pos + (uint8_t)(i * 12u), buf, MID_OLED_FONT_12);
     *     }
     * }
     * ============================================= */

    /* 新增：显示运行时间（12 号字体大字秒数）
     * 计时由 MID_Chassis_Start 启动、MID_Chassis_Stop 停止（手动急停/传感器触发都覆盖）
     * 布局：
     *   第 0-1 行 (y=0, size=12)："TIME:"  标签
     *   第 2-3 行 (y=2, size=12)：秒数 + "s"
     * Clear 清除原显示残留内容，再重绘运行时间
     */
    MID_OLED_Clear();
    MID_OLED_ShowString(0u, 0u, "TIME:", MID_OLED_FONT_12);
    float sec = (float)MID_Chassis_GetRunTimeMs() / 1000.0f;
    MID_OLED_ShowFloat(2u, 0u, sec, 3u, MID_OLED_FONT_12);
    MID_OLED_ShowString(2u, 48u, "s", MID_OLED_FONT_12);

    /* K1 模式运行时额外显示累计里程（mm）
     * 第 4-5 行显示 "DIST:" + 里程数值 + "mm"
     * 非 K1 模式（IDLE/K2）不显示里程，保留原有时间显示
     */
//    if (MID_Key_GetLaunchMode() == MID_MODE_K1) {
//        MID_OLED_ShowString(4u, 0u, "DIST:", MID_OLED_FONT_12);
//        float odom_mm = MID_Encoder_GetOdomMm();
//        MID_OLED_ShowFloat(6u, 0u, odom_mm, 1u, MID_OLED_FONT_12);
//        MID_OLED_ShowString(6u, 60u, "mm", MID_OLED_FONT_12);
//    }
}

/* ============================================================
 *  主函数
 * ============================================================ */

int main(void)
{
    /* 1. 系统初始化（80MHz 主频） */
    LQ_System_Init();
    delay_ms(10);

    /* 2. 初始化竞速 PID 参数（默认急停，闭环启控层数） */
    MID_Param_Init();

    /* 3. 初始化调试串口 UART0 @ 9600 */
    APP_DEBUG_Init();

    /* 4. 初始化电动 PWM（10kHz） */
    MID_Motor_Init(10000);

    /* 5. 初始化编码器 */
    MID_Encoder_Init();

    /* 6. 8 路红外传感器 GPIO 初始化（数字量模式读取） */
    MID_IR_Init();

    /* 7. LSM6DSV16X IMU 初始化（SPI 通信） */
    uint8_t imu_id = LQ_LSM6DSV16X_Init();
    if (imu_id != LSM6DSV16X_DRV_ID) {
        while (1) {
            APP_DEBUG_Printf("IMU Init FAIL, ID=0x%02X\r\n", imu_id);
            APP_DEBUG_ServiceTx();
            delay_ms(500);
        }
    }
    delay_ms(100);

    /* 8. 电动 PID 状态初始化 */
    MID_Chassis_Init();

    /* 9. 初始化调度器（SysTick 1ms 定时器中断） */
    APP_Scheduler_Init();

    /* 10. 按键 + OLED 初始化 */
    MID_Key_Init();
    MID_OLED_Init();

    /* 11. 蜂鸣器初始化 + 上电提示音（2 声"哔"） */
    MID_Beep_Init();
    MID_Beep_Beep(2, 0.08f, 0.08f);

    /* 12. 主循环：渲染 + __WFI */
    while (1) {
        /* TX DMA 推进：从环形队列提交一帧数据到 DMA */
        APP_DEBUG_ServiceTx();

        /* RX 轮询：读取 RX FIFO 并解析 FireWater 协议 */
        APP_DEBUG_PollRx();

        /* 调度器：每 5ms 帧执行控制（8 路红外读取 + 紧急检测 + 速度环） */
        APP_Scheduler_Run();

        /* VOFA+ 绘图输出 */
        APP_DEBUG_PollPlot();

        /* 紧急停车触发后的报警处理（由调度器内部触发） */
        /* 紧急停车标志由 mid_line.c 置位，K1 按键清除 */

        /* 按键周期扫描（20ms 一次） */
        MID_Key_Scan();

        /* OLED 屏幕刷新（50ms 一次） */
        s_screen_refresh();

        /* OLED 分片刷屏：每次 1 页 128 字节，8 页约 40ms */
        MID_OLED_Task();

        /* 再次推进 TX DMA */
        APP_DEBUG_ServiceTx();

        /* 回调到 SysTick 1ms 定时器中断 */
        __WFI();
    }
}
