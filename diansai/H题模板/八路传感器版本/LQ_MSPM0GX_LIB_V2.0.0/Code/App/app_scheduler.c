#include "app_scheduler.h"

/*******************************************************************************
 * @file    app_scheduler.c
 * @brief   5ms 定时调度核心模块（应用层）
 *
 * @note    本文件是系统的核心调度模块，负载应用层
 *          - SysTick 1ms 中断累加 s_app_tick_ms
 *          - 主循环每 5ms 调用 APP_Scheduler_Run() 执行一帧
 *          - 实控逻辑在 s_scheduler_step(dt) 中实现
 *
 *          s_scheduler_step 一帧完成：
 *              1) 8 路线采集 + 偏航环 + 位置环 PD
 *              2) 信号链 Z 轴原始值 + 偏航环 PI（内部使用 g_param.gyro_offset 校准）
 *              3) SFLP 滤波 yaw -> g_chassis.yaw_deg
 *              4) 编码器取速 + 速度环 PI x2 -> PWM 输出
 *
 *          dt 由 ms 转换为秒，传给 PID 控制器，SysTick 1ms 中断保证 PID 采样频率稳定
 *          即使某帧发生延迟，dt 也会相应增大
 *
 *          每帧均检查 g_param.enable_pos / enable_yaw / enable_speed 标志
 *******************************************************************************/

#include "mid_encoder.h"
#include "mid_chassis.h"
#include "mid_motor.h"
#include "mid_pid.h"
#include "mid_imu.h"             /* 信号链接口，附近状态循环 */
#include "mid_line.h"            /* 8 路线 -> 偏差 e_line */
/* LQ_tracking.h 移除控制器8 路选择原 8 路模块删除 */

/* SysTick 1ms 中断的 LOAD 值：80MHz / 80000 = 1kHz = 1ms */
#define SYSTICK_LOAD_1MS   (80000U)

/* 全局系统计数器，由 SysTick 1ms 中断累加
 * 范围 0~约 49.7 天（uint32_t）无溢出风险 */
static volatile uint32_t s_app_tick_ms = 0U;

/* 上一帧执行时刻的 tick 值，用于计算实际dt */
static uint32_t s_last_tick_ms = 0U;

/* 当帧控制执行后面：采集数据 + 偏航环 + 位置环 + 输出 PWM */
static void s_scheduler_step(float dt_s);

/* ============================================================
 *  SysTick 1ms 中断服务程序
 * ============================================================ */
void SysTick_Handler(void)
{
    /* 每个 tick 增加一次，便于各模块使用 */
    s_app_tick_ms++;
}

/* ============================================================
 *  初始化调度器：配置 SysTick 为 1ms 中断
 * ============================================================ */
void APP_Scheduler_Init(void)
{
    /* 先关闭 SysTick，再设置 LOAD 值并使能中断 */
    DL_SYSTICK_init(SYSTICK_LOAD_1MS);          /* LOAD=80000，每 1ms 中断一次 */
    DL_SYSTICK_enableInterrupt();               /* 使能 SysTick 中断 */
    DL_SYSTICK_enable();

    /* 复位计数器初值 */
    __disable_irq();
    s_app_tick_ms  = 0U;
    s_last_tick_ms = 0U;
    __enable_irq();
}

/* ============================================================
 *  主循环中调用的调度器入口，每 5ms 执行一次控制帧
 * ============================================================ */
void APP_Scheduler_Run(void)
{
    uint32_t tick;
    uint32_t elapsed_ms;

    /* 获取当前 tick 值，与上一帧比较计算 dt */
    tick = s_app_tick_ms;
    elapsed_ms = tick - s_last_tick_ms;

    /* 未达 5ms 则直接返回，主循环可进入低功耗模式 */
    if (elapsed_ms < APP_SCHED_TICK_MS) {
        return;
    }

    /* 更新 last_tick 为当前 tick，确保下一帧计时准确（不重下） */
    s_last_tick_ms = tick;

    /* 执行控制帧，dt 单位为秒 */
    s_scheduler_step((float)elapsed_ms / 1000.0f);
}

/* 获取当前系统时间（毫秒），供 VOFA / 校准 / 维护使用 */
uint32_t APP_Scheduler_GetTickMs(void)
{
    return s_app_tick_ms;
}

/* ============================================================
 *  单帧控制函数：数据采集 + PID 运算 + PWM 输出
 *  @param dt_s  实际帧间隔时间（秒）
 *  @note        执行顺序
 *                1) 读取数据 -> 计算偏差 -> 位置环 -> 目标速度
 *                2) 读取角速度 Z 轴 -> 偏航环 -> 左右控制量
 *                3) mid_imu 的 SFLP 滤波 yaw ，用于显示/观测
 *                4) 读取编码器 -> 速度环 -> PWM 输出
 *  @note        所有原始值 / SFLP yaw / 偏差校准
 *               全部通过 mid_imu 接口获取，不在本文件直接操作硬件
 * ============================================================ */
static void s_scheduler_step(float dt_s)
{
    /* 0) 8 路传感器原始电平获取（每 5ms 一次）
     *    8 路 GPIO 读取耗时 < 1us，不影响 5ms 周期实时性
     */
    uint8_t ir_levels[MID_IR_SENSOR_COUNT];
    /* 1) 读取 8 路 -> 计算偏差 e_line -> 位置环 PD 得到 target_omega
     *    注意：某些硬件平台需要先调用查询函数刷新 ADC 值 */
    MID_IR_ReadRaw(ir_levels);          // 8 路原始 H/L 电平
    MID_IR_UpdateBits(ir_levels);       // 8 路 -> bits[8]
		float e_line = MID_Line_CalcError();
    MID_Chassis_PosStep(e_line, dt_s);

    /* 1.5) 8 路全 0/全 1 异常检测（位置环未控制时也能检测）
     *     如 8 路同时同平 -> MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
     *     K1 激活期由 mid_key.c 置标志，这里
     */
    MID_Line_CheckEmergency(ir_levels);

    /* 2) 读取角速度 Z 轴 -> 偏航环 PI 得到 target_diff ，组合速度
     *    YawStep 内部使用 g_param.gyro_offset ，已有偏移校准 */
    float gyro_raw = MID_IMU_ReadGyroZRaw();
    MID_Chassis_YawStep(g_chassis.target_omega, gyro_raw, dt_s);

    /* 3) SFLP 滤波 yaw，每 5ms 刷新一次，避免大初始位置偏移影响
     *    用于下位机显示 / 监测 */
    g_chassis.yaw_deg = MID_IMU_ReadYawDeg();

    /* 4) 读取编码器速度 + 速度环 PID -> PWM 输出
     *    目标速度已由偏航环分配写入 g_chassis.target_speed_l/r */

    // 速度环目标值，偏航环写入 g_chassis.target_speed_l/r）
		//偏航环
//		g_chassis.target_speed_l=g_param.chassis_vx;
//		g_chassis.target_speed_r=g_param.chassis_vx;
    /* 获取实际速度 (mm/s) */
    MID_Encoder_Update();
    float speed_l = MID_Encoder_GetSpeed(MID_ENCODER_LEFT);
    float speed_r = MID_Encoder_GetSpeed(MID_ENCODER_RIGHT);

    /* 速度环控制：输出 PWM 占空比 */
    MID_Chassis_SpeedStep(g_chassis.target_speed_l, g_chassis.target_speed_r,
                          speed_l, speed_r, dt_s);

    /* 5) 速度渐变步进：每 4 帧（4×5ms=20ms）调用一次 MID_Chassis_RampStep
     *    K1 模式里程达阈值时 chassis_vx 递减 10；K2 模式 chassis_vx 递增 10
     *    由 mid_chassis.c 内部判断模式与终点，IDLE 不动作
     */
    {
        static uint8_t s_ramp_div = 0u;   /* 20ms 分频计数器（4 帧=20ms） */
        s_ramp_div++;
        if (s_ramp_div >= 4u) {
            s_ramp_div = 0u;
            MID_Chassis_RampStep();
        }
    }
}
