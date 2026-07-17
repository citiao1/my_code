#include "vehicle_battery.h"

#include "include.h"

#define BATTERY_ADC_CHANNEL       ADC1_Channel_6_Pin_B_19
#define BATTERY_ADC_SAMPLES       8U
#define BATTERY_VDDA_MV           3300U
#define BATTERY_ADC_FULL_SCALE    4095U

/* 原理图：BAT -- 10k -- ADC_bat -- 1.5k -- GND，倍率为 11.5/1.5=23/3。 */
#define BATTERY_DIVIDER_NUMERATOR   23U
#define BATTERY_DIVIDER_DENOMINATOR  3U

static VehicleBatteryState battery_state;

void VehicleBattery_Init(void)
{
    LQConfig_ADC_InitTypeDef_t adc = {
        .clockSel = DL_ADC12_CLOCK_SYSOSC,
        .freqRange = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
        .divideRatio = DL_ADC12_CLOCK_DIVIDE_1,
        .adcclks = 100U,
        .resolution = DL_ADC12_SAMP_CONV_RES_12_BIT,
    };

    memset(&battery_state, 0, sizeof(battery_state));
    LQ_ADC_Init(ADC_Port_1, &adc);
}

void VehicleBattery_Update(void)
{
    uint32_t sum = 0U;
    uint32_t pin_mv;
    uint32_t measured_mv;
    uint8_t sample;

    /* 丢弃切入该通道后的第一份结果，再平均八次，减小电机噪声影响。 */
    (void)LQ_ADC_GetValue(BATTERY_ADC_CHANNEL);
    for (sample = 0U; sample < BATTERY_ADC_SAMPLES; sample++)
    {
        sum += LQ_ADC_GetValue(BATTERY_ADC_CHANNEL);
    }
    battery_state.raw = (uint16_t)(sum / BATTERY_ADC_SAMPLES);

    pin_mv = ((uint32_t)battery_state.raw * BATTERY_VDDA_MV +
              BATTERY_ADC_FULL_SCALE / 2U) / BATTERY_ADC_FULL_SCALE;
    measured_mv = (pin_mv * BATTERY_DIVIDER_NUMERATOR +
                   BATTERY_DIVIDER_DENOMINATOR / 2U) /
                  BATTERY_DIVIDER_DENOMINATOR;

    if (!battery_state.valid)
    {
        battery_state.voltage_mv = measured_mv;
        battery_state.valid = 1U;
    }
    else
    {
        /* 新值权重 1/4，既能抑制 PWM 纹波，也能在约 400 ms 内反映压降。 */
        battery_state.voltage_mv =
            (battery_state.voltage_mv * 3U + measured_mv + 2U) / 4U;
    }
}

const VehicleBatteryState *VehicleBattery_GetState(void)
{
    return &battery_state;
}
