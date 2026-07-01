#ifndef BSP_CAN_H
#define BSP_CAN_H

// CAN 轻量封装：
// 使用步骤：
// 1) 填写 CAN_Init_Config_s（tx_id/rx_id/回调/id）
// 2) 调用 CANRegister() 获取 CANInstance 指针
// 3) 填充 instance->txbuff 后，调用 CANTransmit() 发送
// 4) 接收中断中按 rx_id 自动分发，触发 can_module_callback()


#include "main.h"
#include "can.h"
#include "stdlib.h"

#pragma pack(1)

// CAN 实例：
// - 每个上层模块注册一个实例，通过 rx_id 过滤并回调
// - id 指向上层对象（如电机实例），回调时回传
typedef struct _
{
    CAN_TxHeaderTypeDef tx_config;   // 发送报文头（StdId/DLC/IDE/RTR）
    uint32_t tx_id;                  // 发送 ID（标准帧）
    uint32_t rx_id;                  // 接收 ID（标准帧）
    uint8_t txbuff[8];               // 发送缓冲区
    uint8_t rxbuff[8];               // 接收缓冲区
    uint32_t txmailbox;              // 发送邮箱句柄

    void *id;                        // 上层对象指针（用户自定义）
    void (*can_module_callback)(struct _ *);  // 接收回调（按 rx_id 匹配）
}CANInstance;

// 实例初始化配置
typedef struct 
{
    uint32_t tx_id;                                  // 发送 ID
    uint32_t rx_id;                                  // 接收 ID
    void (*can_module_callback)(CANInstance *);      // 接收回调
    void *id;                                        // 上层对象指针
} CAN_Init_Config_s;

#pragma pack(0)

// 注册 CAN 实例并返回句柄
CANInstance *CANRegister(CAN_Init_Config_s *config);
// 发送报文（timeout 为毫秒 tick，返回 1 成功/0 失败）
uint8_t CANTransmit(CANInstance *_instance, float timeout);

#endif // !BSP_CAN_H
