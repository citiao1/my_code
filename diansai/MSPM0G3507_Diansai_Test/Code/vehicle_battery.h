#ifndef VEHICLE_BATTERY_H
#define VEHICLE_BATTERY_H

#include <stdint.h>

typedef struct
{
    uint16_t raw;
    uint32_t voltage_mv;
    uint8_t valid;
} VehicleBatteryState;

/* 初始化原理图中的 PB19 / ADC1_CH6 电池分压检测。 */
void VehicleBattery_Init(void);

/* 八次平均并更新低通后的电池端电压，建议每 100 ms 调用一次。 */
void VehicleBattery_Update(void);

/* 返回内部只读状态；voltage_mv 是电池输入端估算值，不是 ADC 引脚电压。 */
const VehicleBatteryState *VehicleBattery_GetState(void);

#endif
