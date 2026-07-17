#ifndef VEHICLE_GRAY_H
#define VEHICLE_GRAY_H

#include <stdint.h>

#define VEHICLE_GRAY_CHANNELS 8U
#define VEHICLE_GRAY_NORMALIZED_MAX 1000U

typedef struct
{
    /*
     * 为保持现有 CAL/STA 协议，下列数组仍沿用 white/black 字段名；实际语义为
     * white=背景参考、black=目标线参考。白底黑线和蓝底白线共用同一套算法。
     */
    uint16_t raw[VEHICLE_GRAY_CHANNELS];
    uint16_t white[VEHICLE_GRAY_CHANNELS];
    uint16_t black[VEHICLE_GRAY_CHANNELS];
    /* normalized 始终以背景为 0、要追踪的线为 1000，并限制在 0..1000。 */
    uint16_t normalized[VEHICLE_GRAY_CHANNELS];
    uint8_t white_valid;
    uint8_t black_valid;
    uint8_t normalization_valid;
} VehicleGrayState;

/* 初始化 S0/S1/S2 多路选择和 ADC 轮询接口。 */
void VehicleGray_Init(void);

/* 依次读取八路灰度；每路丢弃首样本并平均后续四个样本。 */
void VehicleGray_Update(void);

/* 清除两项标定和归一化结果；切换赛道颜色模式时必须调用。 */
void VehicleGray_ResetCalibration(void);

/* 立即重新采样并保存背景参考；归一化有效时返回 1。 */
uint8_t VehicleGray_CaptureBackground(void);

/* 立即重新采样并保存目标线参考；归一化有效时返回 1。 */
uint8_t VehicleGray_CaptureLine(void);

/* 兼容旧调用名：分别等价于 CaptureBackground/CaptureLine。 */
uint8_t VehicleGray_CaptureWhite(void);

uint8_t VehicleGray_CaptureBlack(void);

/* 统计当前 raw 中大于等于 threshold 的通道数。 */
/* 按归一化值统计被目标线覆盖的通道；未完成有效标定时返回 0。 */
uint8_t VehicleGray_CountLineChannels(uint16_t normalized_threshold);

/* 返回模块内部只读快照；调用者不得修改标定数组。 */
const VehicleGrayState *VehicleGray_GetState(void);

#endif
