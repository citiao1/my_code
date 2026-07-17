#include "vehicle_encoder.h"

#include "include.h"
#include "LQ_device.h"

/* 当前驱动只统计 A 相下降沿，下面数值来自本车直接行驶 1 m 的标定。 */
#define LEFT_COUNTS_PER_METER   1867
#define RIGHT_COUNTS_PER_METER  1867
#define DEFAULT_SAMPLE_MS         10U
#define FILTER_NEW_WEIGHT          0.35f

static VehicleEncoderState encoder_state;
static LQConfig_Encoder_InitTypeDef_t left_config = {
    .pinA = GPIO_Pin_A_7,
    .pinB = GPIO_Pin_A_3,
    .encoder_cnt = 0,
    .count = 0,
    .gpio_flag = 0,
};
static LQConfig_Encoder_InitTypeDef_t right_config = {
    .pinA = GPIO_Pin_A_8,
    .pinB = GPIO_Pin_B_7,
    .encoder_cnt = 0,
    .count = 0,
    .gpio_flag = 0,
};

static int32_t RoundFloat(float value)
{
    return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

void VehicleEncoder_Init(void)
{
    memset(&encoder_state, 0, sizeof(encoder_state));
    LQ_Encoder_Init(500U, &left_config);
    LQ_Encoder_Init(500U, &right_config);
    VehicleEncoder_Reset();
}

void VehicleEncoder_Reset(void)
{
    /* 中断可能同时更新计数，清零配置和软件快照必须处于同一临界区。 */
    __disable_irq();
    left_config.encoder_cnt = 0;
    left_config.count = 0;
    right_config.encoder_cnt = 0;
    right_config.count = 0;
    memset(&encoder_state, 0, sizeof(encoder_state));
    __enable_irq();
}

void VehicleEncoder_Update(uint32_t elapsed_ms)
{
    int32_t left_delta;
    int32_t right_delta;
    float raw_left;
    float raw_right;

    __disable_irq();
    left_delta = left_config.encoder_cnt + left_config.count;
    right_delta = right_config.encoder_cnt + right_config.count;
    left_config.encoder_cnt = 0;
    left_config.count = 0;
    right_config.encoder_cnt = 0;
    right_config.count = 0;
    __enable_irq();

    encoder_state.total_left += left_delta;
    encoder_state.total_right += right_delta;
    if (elapsed_ms == 0U) elapsed_ms = DEFAULT_SAMPLE_MS;

    /*
     * 必须使用本次真实间隔。若任务被阻塞后仍固定按 10 ms 换算，累计脉冲会
     * 被误判成速度尖峰，外层 PID 随后会错误地削减电机输出。
     */
    raw_left = (float)left_delta * 1000000.0f /
               ((float)LEFT_COUNTS_PER_METER * (float)elapsed_ms);
    raw_right = (float)right_delta * 1000000.0f /
                ((float)RIGHT_COUNTS_PER_METER * (float)elapsed_ms);

    encoder_state.filtered_left_mm_s_float =
        (1.0f - FILTER_NEW_WEIGHT) * encoder_state.filtered_left_mm_s_float +
        FILTER_NEW_WEIGHT * raw_left;
    encoder_state.filtered_right_mm_s_float =
        (1.0f - FILTER_NEW_WEIGHT) * encoder_state.filtered_right_mm_s_float +
        FILTER_NEW_WEIGHT * raw_right;

    encoder_state.raw_left_mm_s = RoundFloat(raw_left);
    encoder_state.raw_right_mm_s = RoundFloat(raw_right);
    encoder_state.filtered_left_mm_s =
        RoundFloat(encoder_state.filtered_left_mm_s_float);
    encoder_state.filtered_right_mm_s =
        RoundFloat(encoder_state.filtered_right_mm_s_float);
}

const VehicleEncoderState *VehicleEncoder_GetState(void)
{
    return &encoder_state;
}
