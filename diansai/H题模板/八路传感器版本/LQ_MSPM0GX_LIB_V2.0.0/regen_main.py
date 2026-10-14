"""
生成 GB2312 编码的 main.c
- 上电默认急停
- K1 短按：mid_key.c 内部校准陀螺仪 + 启动底盘
- K1 再按：急停
- 主循环最后调 s_oled_adc_refresh() 每 50ms 刷新 8 路 ADC 值到 OLED
  （MID_OLED_Task() 在 main loop 末尾做分片刷屏）
"""
import codecs

CONTENT = r'''/*******************************************************************************
 * @file    main.c
 * @brief   主程序 - 差速底盘三环 PID 控制
 *
 * @note    上电初始化流程：
 *          1. 系统初始化（80MHz 主频）
 *          2. 初始化全局参数（PID 默认值 + 默认急停状态）
 *          3. 初始化调试串口（UART0 @ 9600 波特率，VOFA+ 用）
 *          4. 初始化电机 PWM（10kHz）
 *          5. 初始化编码器 + 按键
 *          6. 初始化灰度（8 路循迹 ADC）
 *          7. 初始化 LSM6DSV16X（SPI 接口，全走 mid_imu 中间层）
 *          8. 初始化 SFLP 6 轴姿态融合 + 记录上电 yaw 零位
 *          9. 初始化底盘 PID 状态
 *         10. 初始化调度器（SysTick 1ms 中断 + 5ms 调度）
 *         11. 初始化 OLED 0.96 寸 + 分片刷屏模式
 *         12. 蜂鸣器上电提示（2 声"哔"，间隔 0.5s）
 *         13. 主循环：ServiceTx + PollRx + Scheduler_Run + PollPlot + OLED_ADC + OLED_Task
 *
 *          主循环按"高频轮询 + __WFI 省电"模式：
 *          - SysTick 每 1ms 中断唤醒 CPU
 *          - APP_DEBUG_ServiceTx    推进 TX DMA（缓冲发送出去）
 *          - APP_DEBUG_PollRx       轮询 RX FIFO，拼装 FireWater 协议帧
 *          - APP_Scheduler_Run      内部判断 5ms 帧是否到，未到直接返回
 *          - APP_DEBUG_PollPlot     内部 20ms 周期发送绘图数据
 *          - s_oled_adc_refresh     每 50ms 读 8 路 ADC 并写到 OLED GRAM（置 dirty）
 *          - MID_OLED_Task          每次 main loop 刷 1 页 128B，dirty=0 时 0 开销
 *
 *          上电默认急停（使能位全 0 + 速度 0 + 电机停转）
 *          按 K1 短按 = mid_key.c 内部校准陀螺仪 + 启动流程
 *          再按 K1 = 急停（清 PID + 清积分 + 速度清零 + 电机停转）
 *
 *          OLED 显示 8 路灰度 ADC（0~100 归一化值）：
 *            Row 0: S0=  VVV
 *            Row 1: S1=  VVV
 *            ...
 *            Row 7: S7=  VVV
 *******************************************************************************/

#include "include.h"
#include <stdio.h>                  /* snprintf 用于 ADC 值格式化 */
#include "LQ_device.h"
#include "LQ_tracking.h"
#include "mid_motor.h"
#include "mid_encoder.h"
#include "mid_chassis.h"
#include "mid_pid.h"
#include "mid_imu.h"
#include "app_debug.h"
#include "app_scheduler.h"
#include "mid_beep.h"
#include "mid_key.h"
#include "mid_oled.h"               /* OLED 0.96 寸 显示中间层（分片刷屏） */

/* ============================================================
 *  OLED 8 路 ADC 周期性刷新
 *  - 50ms 一次（~20Hz 屏幕刷新，对人眼无抖动）
 *  - 调用 LQ_Tracking_Polling_GetValue 读 8 路灰度（0~100 归一化）
 *  - 8 行分别 snprintf 后调 MID_OLED_ShowString
 *  - ShowString 内部置 dirty=1 + page=0
 *  - 后续 main loop 的 MID_OLED_Task() 会分片把 8 页全刷一遍
 *  - LQ_Tracking_Polling_GetValue 内部 8 路 × 5 次 ADC（去头 3 取后 2 平均），
 *    单次约 200us，在 main loop 跑不影响 5ms 调度
 * ============================================================ */
static uint32_t s_oled_adc_last_ms = 0U;   /* 上次刷新的 SysTick tick（ms） */
#define OLED_ADC_REFRESH_MS  (50U)          /* 刷新周期 */

static void s_oled_adc_refresh(void)
{
    uint32_t now_ms;
    char buf[8];
    uint8_t i;

    now_ms = APP_Scheduler_GetTickMs();
    if ((now_ms - s_oled_adc_last_ms) < OLED_ADC_REFRESH_MS) {
        return;     /* 未到刷新周期，直接返回 */
    }
    s_oled_adc_last_ms = now_ms;

    /* 读 8 路灰度 ADC（值域 0~100） */
    LQ_Tracking_Polling_GetValue();

    /* 8 行写 GRAM：S<idx>=<value>，value 3 位右对齐空格补 0 */
    for (i = 0U; i < 8U; i++) {
        snprintf(buf, sizeof(buf), "S%u=%3u",
                 (unsigned)i, (unsigned)LQ_Tracking_Value[i]);
        MID_OLED_ShowString(i, 0, buf, MID_OLED_FONT_8);
    }
    /* 8 次 ShowString 都置 dirty=1，MID_OLED_Task() 会从头刷 8 页 */
}

int main(void)
{
    /* 1. 系统初始化（80MHz 主频） */
    LQ_System_Init();
    delay_ms(10);

    /* 2. 全局参数初始化（PID 默认值 + 上电默认急停状态） */
    MID_Param_Init();

    /* 3. 调试串口 UART0 @ 9600 初始化 */
    APP_DEBUG_Init();

    /* 4. 电机 PWM 初始化（10kHz） */
    MID_Motor_Init(10000);

    /* 5. 编码器初始化 + 按键中间层初始化 */
    MID_Encoder_Init();
    MID_Key_Init();

    /* 6. 灰度初始化（8 路循迹 ADC） */
    LQ_Tracking_Polling_Init();

    /* 7. LSM6DSV16X 初始化（SPI 接口，全走 mid_imu 中间层） */
    if (MID_IMU_Init() != MID_IMU_OK) {
        /* IMU 初始化失败：循环串口打印错误，方便现场排查 */
        while (1) {
            APP_DEBUG_Printf("IMU Init FAIL, ID=0x%02X\r\n", 0);
            APP_DEBUG_ServiceTx();
            delay_ms(500);
        }
    }
    delay_ms(100);  /* 上电后硬件稳定（参考官方 demo） */

    /* 陀螺仪 Z 轴零偏校准：
     *   上电默认不自动校准（避免车一上电就"跑"出去）
     *   首次按 K1 时由 mid_key.c 内部校准 */

    /* 初始化 SFLP 6 轴姿态融合，并记录当前 yaw 零位 */
    if (MID_IMU_SFLPInitAndCalibrateYaw() != MID_IMU_OK) {
        while (1) {
            APP_DEBUG_Printf("IMU SFLP Init FAIL\r\n");
            APP_DEBUG_ServiceTx();
            delay_ms(500);
        }
    }
    /* SFLP 硬件约 1s 稳定，main 循环里依然刷新 */

    /* 8. 底盘 PID 状态初始化（清 PID 历史） */
    MID_Chassis_Init();

    /* 9. 调度器初始化（SysTick 1ms 中断 + 5ms 调度） */
    APP_Scheduler_Init();

    /* 10. OLED 0.96 寸初始化
     *     - 5 个 GPIO 推挽输出 + 复位 + SSD1306 命令 + 清 GRAM（dirty=1）
     *     - 共 100ms 左右，期间 main loop 未启动
     *     - 首次 MID_OLED_Task() 把全黑 GRAM 推到屏幕 */
    MID_OLED_Init();

    /* 11. 蜂鸣器上电提示（2 声"哔"，间隔 0.5s）
     *     每次上电/复位时响一次，确认硬件正常
     *     提示约 2.6s，期间不进入主循环
     *     注意：提示未完成时即便控制逻辑未就绪也不会异常 */
    MID_Beep_Init();
    MID_Beep_Beep(2, 0.7f, 0.5f);

    /* 12. 主循环（高频轮询 + __WFI 省电） */
    while (1) {
        /* TX DMA 推进（将缓冲数据提交一帧数据到 DMA） */
        APP_DEBUG_ServiceTx();

        /* RX 轮询（每次读 RX FIFO，拼装 FireWater 协议帧） */
        APP_DEBUG_PollRx();

        /* 底盘调度（内部 5ms 帧执行，未到直接返回） */
        APP_Scheduler_Run();

        /* 绘图（内部 20ms 周期） */
        APP_DEBUG_PollPlot();

        /* 再次推进 TX DMA，确保新产生的数据及时发出去 */
        APP_DEBUG_ServiceTx();

        /* 按键轮询（20ms 一次） */
        MID_Key_Scan();

        /* OLED 8 路 ADC 周期性刷新：50ms 一次读 + 8 行写 GRAM
         *   写 GRAM 仅修改 RAM + 置 dirty，不阻塞
         *   真正的 SPI 发送由下一步 MID_OLED_Task() 分片完成 */
        s_oled_adc_refresh();

        /* OLED 分片刷屏：每次只刷 1 页 128B 约 ~0.5ms 阻塞（dirty=0 时 0 开销）
         *   5ms 调度下约 0.5ms / 5ms = 10% 占空比，不影响 5ms tick
         *   8 页全刷 8 次 main loop = 40ms = 25Hz 屏幕刷新率 */
        MID_OLED_Task();

        /* 省电：等待一个 SysTick 中断（1ms）唤醒，降低功耗 */
        __WFI();
    }
}
'''

# 用 GB2312 编码写入（按用户硬性要求所有源文件必须是 GB2312）
with codecs.open(r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'w', encoding='gb2312') as f:
    f.write(CONTENT)

print("main.c written, size:", len(CONTENT.encode('gb2312')))
