/*******************************************************************************
 * @file    mid_encoder.c
 * @brief   编码器中间层实现
 *
 * @note    封装 LQ_Encoder，将原始计数值转换为实际速度 (mm/s)
 *          每个编码器对应一个 LQConfig_Encoder_InitTypeDef_t 实例
 *
 *          信号通路：
 *          编码 A 相 + B 相 -> 外部中断 4 倍频累计 -> 定时读取计数值
 *          -> 1e6/(3726*5ms) = mm/s -> IIR 低通 -> 喂给 PID
 *
 *          其中 3726 为实际标定值，1e6 来源于 (米*1000) + 毫秒转换(*1000)
 *******************************************************************************/

#include "mid_encoder.h"

/* 编码器配置结构体实例：左/右各一路，与 LQ 编码器一一对应 */
static LQConfig_Encoder_InitTypeDef_t s_enc_left;
static LQConfig_Encoder_InitTypeDef_t s_enc_right;

/* 累计脉冲数（跑完标定刚好1米后看编码器 = 脉冲/米） */
static int32_t s_total[2] = {0, 0};

/* ============================================================
 *  速度计算 + IIR 滤波
 * ============================================================ */

/* 滤波后速度（mm/s），供外部 PID 使用 */
static float s_speed_mmps[2] = {0.0f, 0.0f};

void MID_Encoder_Init(void)
{
    /* 左编码器 */
    s_enc_left.pinA = MID_ENCODER_LEFT_PINA;
    s_enc_left.pinB = MID_ENCODER_LEFT_PINB;
    s_enc_left.encoder_cnt = 0;
    s_enc_left.count = 0;
    s_enc_left.gpio_flag = 0;
    LQ_Encoder_Init(MID_ENCODER_SAMPLE_MS, &s_enc_left);

    /* 右编码器 */
    s_enc_right.pinA = MID_ENCODER_RIGHT_PINA;
    s_enc_right.pinB = MID_ENCODER_RIGHT_PINB;
    s_enc_right.encoder_cnt = 0;
    s_enc_right.count = 0;
    s_enc_right.gpio_flag = 0;
    LQ_Encoder_Init(MID_ENCODER_SAMPLE_MS, &s_enc_right);

    /* 清除速度和累计 */
    s_speed_mmps[MID_ENCODER_LEFT]  = 0.0f;
    s_speed_mmps[MID_ENCODER_RIGHT] = 0.0f;
    s_total[MID_ENCODER_LEFT]  = 0;
    s_total[MID_ENCODER_RIGHT] = 0;
}

/* 每 5ms 调用一次，从编码器读取累计计数值，转 mm/s、IIR 滤波、累计脉冲
 * @note  可由调度器 (app_scheduler.c) 在 5ms 任务周期内调用
 *        必须在调用 PID 计算之前调用
 */
void MID_Encoder_Update(void)
{
    int32_t left_delta, right_delta;

    /* 关中断读取原始编码值并清零，防止中断打断导致数据撕裂 */
    __disable_irq();
    left_delta  = s_enc_left.encoder_cnt + s_enc_left.count;
    right_delta = s_enc_right.encoder_cnt + s_enc_right.count;
    s_enc_left.encoder_cnt = 0;
    s_enc_left.count = 0;
    s_enc_right.encoder_cnt = 0;
    s_enc_right.count = 0;
    __enable_irq();

    /* 累计脉冲 */
    s_total[MID_ENCODER_LEFT]  += left_delta;
    s_total[MID_ENCODER_RIGHT] += right_delta;

    /* 计算瞬时速度 (mm/s)
     * 距离 = delta / COUNTS_PER_METER 米
     * 时间 = SAMPLE_MS / 1000 秒
     * 速度 = 距离 / 时间 = delta * 1000 / (COUNTS_PER_METER * SAMPLE_MS / 1000)
     *      = delta * 1000000 / (COUNTS_PER_METER * SAMPLE_MS)  mm/s
     */
    float raw_l = (float)left_delta  * 1000000.0f /
                  (MID_ENCODER_COUNTS_PER_METER * (float)MID_ENCODER_SAMPLE_MS);
    float raw_r = (float)right_delta * 1000000.0f /
                  (MID_ENCODER_COUNTS_PER_METER * (float)MID_ENCODER_SAMPLE_MS);

    /* 一阶 IIR 低通滤波：filtered = (1-alpha)*old + alpha*raw */
    s_speed_mmps[MID_ENCODER_LEFT]  = (1.0f - MID_ENCODER_IIR_ALPHA) * s_speed_mmps[MID_ENCODER_LEFT]
                                    + MID_ENCODER_IIR_ALPHA * raw_l;
    s_speed_mmps[MID_ENCODER_RIGHT] = (1.0f - MID_ENCODER_IIR_ALPHA) * s_speed_mmps[MID_ENCODER_RIGHT]
                                    + MID_ENCODER_IIR_ALPHA * raw_r;
}

/* 获取滤波后速度，单位 mm/s
 * @param enc  左/右编码器
 * @return     速度（正=前进，负=后退）
 * @note       必须先调用 MID_Encoder_Update()
 */
float MID_Encoder_GetSpeed(MID_Encoder_t enc)
{
    return s_speed_mmps[enc];
}

/* 获取累计脉冲数
 * @param enc  左/右编码器
 * @return     从开机累计的总脉冲数
 */
int32_t MID_Encoder_GetTotal(MID_Encoder_t enc)
{
    return s_total[enc];
}


/* ============================================================
 *  里程计算（基于编码器脉冲差分，无积分漂移）
 * ============================================================ */

/* 里程基准脉冲值：记录 OdomReset 调用时刻的左右轮累计脉冲
 * 后续里程 = ((total_l - start_l) + (total_r - start_r)) / 2 / COUNTS_PER_METER * 1000  (mm)
 * 未调用 OdomReset 前保持为 0，此时 GetOdomMm 返回的是绝对累计里程
 */
static int32_t s_odom_start_l = 0;
static int32_t s_odom_start_r = 0;

/* 里程归零：记录当前左右轮累计脉冲为基准
 * @note  供 K1 发车模式调用，发车时里程从 0 开始累计
 *        内部记录 MID_Encoder_GetTotal(LEFT/RIGHT) 当前值作为基准
 */
void MID_Encoder_OdomReset(void)
{
    s_odom_start_l = MID_Encoder_GetTotal(MID_ENCODER_LEFT);
    s_odom_start_r = MID_Encoder_GetTotal(MID_ENCODER_RIGHT);
}

/* 查询累计行驶里程（mm）
 * @return 从上次 OdomReset 起的累计里程（mm），左右轮取平均
 *         计算式：((total_l - start_l) + (total_r - start_r)) / 2 / COUNTS_PER_METER * 1000
 * @note  基于脉冲差分，无积分漂移，精度高
 *        未调用 OdomReset 前基准为 0，返回的是绝对累计里程
 */
float MID_Encoder_GetOdomMm(void)
{
    int32_t total_l = MID_Encoder_GetTotal(MID_ENCODER_LEFT);
    int32_t total_r = MID_Encoder_GetTotal(MID_ENCODER_RIGHT);
    float diff_l = (float)(total_l - s_odom_start_l);
    float diff_r = (float)(total_r - s_odom_start_r);

    /* 左右轮差分取平均，再按 COUNTS_PER_METER 换算为 mm */
    return (diff_l + diff_r) / 2.0f / MID_ENCODER_COUNTS_PER_METER * 1000.0f;
}
