#ifndef VEHICLE_GRAY_H
#define VEHICLE_GRAY_H

#include <stdint.h>

#define VEHICLE_GRAY_CHANNELS 8U
#define VEHICLE_GRAY_NORMALIZED_MAX 1000U

typedef struct
{
    /* raw 为 0..4095 的 ADC 值；white/black 为人工触发保存的参考值。 */
    uint16_t raw[VEHICLE_GRAY_CHANNELS];
    uint16_t white[VEHICLE_GRAY_CHANNELS];
    uint16_t black[VEHICLE_GRAY_CHANNELS];
    /* normalized 以白底为 0、黑线为 1000，并限制在 0..1000。 */
    uint16_t normalized[VEHICLE_GRAY_CHANNELS];
    uint8_t white_valid;
    uint8_t black_valid;
    uint8_t normalization_valid;
} VehicleGrayState;

/* 初始化 S0/S1/S2 多路选择和 ADC 轮询接口。 */
void VehicleGray_Init(void);

/* 依次读取八路灰度；每路丢弃首样本并平均后续四个样本。 */
void VehicleGray_Update(void);

/* 立即重新采样并保存白底参考；归一化有效时返回 1。 */
uint8_t VehicleGray_CaptureWhite(void);

/* 立即重新采样并保存黑线参考；归一化有效时返回 1。 */
uint8_t VehicleGray_CaptureBlack(void);

/* 统计当前 raw 中大于等于 threshold 的通道数。 */
uint8_t VehicleGray_CountActive(uint16_t threshold);

/* 返回模块内部只读快照；调用者不得修改标定数组。 */
const VehicleGrayState *VehicleGray_GetState(void);

#endif
