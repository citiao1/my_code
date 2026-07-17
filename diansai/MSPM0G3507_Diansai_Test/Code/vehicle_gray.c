#include "vehicle_gray.h"

#include "include.h"
#include "LQ_device.h"

#define GRAY_MIN_CALIBRATION_SPAN 32

static VehicleGrayState gray_state;

/*
 * 将当前 ADC 值按每个通道各自的白/黑参考映射到 0..1000：
 *
 *     normalized = (raw - white) * 1000 / (black - white)
 *
 * 分母允许为负，因此无论传感器是“黑大白小”还是“黑小白大”都成立。
 * 任意一路黑白差小于 32 ADC count 时，说明标定对比度不足，整组归一化
 * 标记为无效，防止后续循迹把噪声放大成满量程位置误差。
 */
static uint8_t UpdateNormalization(void)
{
    uint8_t channel;
    uint8_t valid = 1U;

    if (!gray_state.white_valid || !gray_state.black_valid)
    {
        memset(gray_state.normalized, 0, sizeof(gray_state.normalized));
        gray_state.normalization_valid = 0U;
        return 0U;
    }

    for (channel = 0U; channel < VEHICLE_GRAY_CHANNELS; channel++)
    {
        int32_t denominator = (int32_t)gray_state.black[channel] -
                              (int32_t)gray_state.white[channel];
        int32_t numerator;
        int32_t normalized;

        if (denominator > -GRAY_MIN_CALIBRATION_SPAN &&
            denominator < GRAY_MIN_CALIBRATION_SPAN)
        {
            gray_state.normalized[channel] = 0U;
            valid = 0U;
            continue;
        }

        numerator = ((int32_t)gray_state.raw[channel] -
                     (int32_t)gray_state.white[channel]) *
                    (int32_t)VEHICLE_GRAY_NORMALIZED_MAX;
        normalized = numerator / denominator;
        if (normalized < 0) normalized = 0;
        if (normalized > (int32_t)VEHICLE_GRAY_NORMALIZED_MAX)
        {
            normalized = (int32_t)VEHICLE_GRAY_NORMALIZED_MAX;
        }
        gray_state.normalized[channel] = (uint16_t)normalized;
    }

    gray_state.normalization_valid = valid;
    return valid;
}

void VehicleGray_Init(void)
{
    memset(&gray_state, 0, sizeof(gray_state));
    LQ_Tracking_Polling_Init();
}

void VehicleGray_Update(void)
{
    uint8_t channel;
    uint8_t sample;
    uint32_t sum;

    /*
     * S0/S1/S2 选择一路模拟量。切换后等待 5 us，并丢弃第一次 ADC 结果，
     * 再平均 4 次，降低多路复用器和 ADC 采样保持电容带来的串扰。
     */
    for (channel = 0U; channel < VEHICLE_GRAY_CHANNELS; channel++)
    {
        LQ_GPIO_WritePin(Tracking_S0_PIN, (channel & 0x01U) ? 1 : 0);
        LQ_GPIO_WritePin(Tracking_S1_PIN, (channel & 0x02U) ? 1 : 0);
        LQ_GPIO_WritePin(Tracking_S2_PIN, (channel & 0x04U) ? 1 : 0);
        delay_us(5);

        (void)LQ_ADC_GetValue(Tracking_ADC_CH);
        sum = 0U;
        for (sample = 0U; sample < 4U; sample++)
        {
            sum += LQ_ADC_GetValue(Tracking_ADC_CH);
        }
        gray_state.raw[channel] = (uint16_t)(sum / 4U);
    }
    (void)UpdateNormalization();
}

static void Capture(uint16_t target[VEHICLE_GRAY_CHANNELS])
{
    uint8_t channel;

    VehicleGray_Update();
    for (channel = 0U; channel < VEHICLE_GRAY_CHANNELS; channel++)
    {
        target[channel] = gray_state.raw[channel];
    }
}

uint8_t VehicleGray_CaptureWhite(void)
{
    Capture(gray_state.white);
    gray_state.white_valid = 1U;
    return UpdateNormalization();
}

uint8_t VehicleGray_CaptureBlack(void)
{
    Capture(gray_state.black);
    gray_state.black_valid = 1U;
    return UpdateNormalization();
}

uint8_t VehicleGray_CountActive(uint16_t threshold)
{
    uint8_t channel;
    uint8_t count = 0U;

    for (channel = 0U; channel < VEHICLE_GRAY_CHANNELS; channel++)
    {
        if (gray_state.raw[channel] >= threshold) count++;
    }
    return count;
}

const VehicleGrayState *VehicleGray_GetState(void)
{
    return &gray_state;
}
