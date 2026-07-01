#include "bsp_can.h"
#include "stdio.h"
#include "stdlib.h"
#include <string.h>  // 包含 memset 和 memcpy 函数

// CAN 通信适配层实现：
// - 提供 CAN 过滤器配置与使能
// - 维护一个 CANInstance 指针数组，按 rx_id 分发接收报文
// - 封装发送接口（带超时）与接收回调（FIFO0/FIFO1）


CANInstance *can_instance[16] = {0};
static uint8_t idx;    // 已注册的 CAN 实例数量


// 使能 CAN1/CAN2，并配置过滤器与接收中断
void CanEnable()
{
    CAN_FilterTypeDef can_filter_config;
    CAN_FilterTypeDef can2_filter_config;

    can_filter_config.FilterActivation = ENABLE;
    can_filter_config.FilterBank = 0;
    can_filter_config.FilterFIFOAssignment = CAN_FilterFIFO0;
    can_filter_config.FilterMode = CAN_FILTERMODE_IDMASK;    
    can_filter_config.FilterIdHigh = 0x0000;
    can_filter_config.FilterIdLow = 0x0000;
    can_filter_config.FilterMaskIdHigh = 0x0000;
    can_filter_config.FilterMaskIdLow = 0x0000;
    can_filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter_config.SlaveStartFilterBank = 14;
    can2_filter_config.FilterActivation = ENABLE;
    can2_filter_config.FilterBank = 14;
    can2_filter_config.FilterFIFOAssignment = CAN_FilterFIFO1;
    can2_filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    can2_filter_config.FilterIdHigh = 0x0000;
    can2_filter_config.FilterIdLow = 0x0000;
    can2_filter_config.FilterMaskIdHigh = 0x0000;
    can2_filter_config.FilterMaskIdLow = 0x0000;
    can2_filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
    can2_filter_config.SlaveStartFilterBank = 14;

    // 注意：两个 CAN 的过滤器均配置为全接收（ID/Mask 均为 0x0000）
    HAL_CAN_ConfigFilter(&hcan1, &can_filter_config);
    HAL_CAN_ConfigFilter(&hcan2, &can_filter_config);
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_Start(&hcan2);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO1_MSG_PENDING);

}



// 注册一个 CAN 实例：
// - 由调用者提供 tx/rx ID、回调以及用户对象 id
// - 保存到全局 can_instance 表，供接收时按 rx_id 分发
CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    if(!idx)
    {
        CanEnable();
    }
    if(idx >= 16)
    {
        while(1);
    }
    // 可按需增加重复 rx_id 检测/拒绝策略（目前允许多个实例同 rx_id）
    CANInstance *instance = (CANInstance *)malloc(sizeof(CANInstance));
    memset(instance, 0, sizeof(CANInstance));

    instance->tx_id = config->tx_id;
    instance->rx_id = config->rx_id;
    instance->id = config->id;
    instance->can_module_callback = config->can_moudle_callback;
    instance->tx_config.StdId = config->tx_id;
    instance->tx_config.DLC = 0x08;
    instance->tx_config.IDE = CAN_ID_STD;
    instance->tx_config.RTR = CAN_RTR_DATA;
    instance->tx_config.TransmitGlobalTime = DISABLE;

    can_instance[idx++] = instance;

    return instance;
}

// 发送 CAN 报文：
// - 轮询空闲邮箱，超时返回 0
// - 发送成功返回 1
uint8_t CANTransmit(CANInstance *_instance, float timeout)
{
    uint32_t systim;
    systim = uwTick;
    while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0)
    {
        if(uwTick - systim > timeout)
        {
            return 0;
        }
    }

    if(HAL_CAN_AddTxMessage(&hcan1,&_instance->tx_config,_instance->txbuff,&_instance->txmailbox) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

// 接收中断统一回调（支持 CAN1/CAN2 与 FIFO0/FIFO1）：
// - 从指定 FIFO 读取报文
// - 遍历已注册实例，按 rx_id 匹配并拷贝 rxbuff
// - 调用每个实例的模块回调进行上层解析
void CANFIFOxCallback(CAN_HandleTypeDef *_hcan, uint32_t fifox)
{
    static CAN_RxHeaderTypeDef can_rxconfig;
    static uint8_t can_rxbuff[8];
    static uint8_t can2_rxbuff[8];
    if(_hcan == &hcan1)
    {
        while(HAL_CAN_GetRxFifoFillLevel(&hcan1,fifox))
        {
            HAL_CAN_GetRxMessage(&hcan1,fifox,&can_rxconfig,can_rxbuff);
            for(uint8_t i = 0;i < idx;i++)
            {
                if(can_rxconfig.StdId == can_instance[i]->rx_id)
                {
                    memcpy(can_instance[i]->rxbuff,can_rxbuff,can_rxconfig.DLC);
                    can_instance[i]->can_module_callback(can_instance[i]);
                }
                
            }
        }
    }
    if(_hcan == &hcan2)
    {
        while(HAL_CAN_GetRxFifoFillLevel(&hcan2,fifox))
        {
            HAL_CAN_GetRxMessage(&hcan2,fifox,&can_rxconfig,can2_rxbuff);
            for (uint8_t i = 0; i < idx; i++)
            {
                if (can_rxconfig.StdId == can_instance[i]->rx_id)
                {
                    memcpy(can_instance[i]->rxbuff, can2_rxbuff, can_rxconfig.DLC);
                    can_instance[i]->can_module_callback(can_instance[i]);
                }
            }
        }
    }
}

// HAL 回调转发：FIFO0（CAN1/CAN2 通用）
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CANFIFOxCallback(hcan, CAN_RX_FIFO0);
}

// HAL 回调转发：FIFO1（CAN1/CAN2 通用）
void Hal_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CANFIFOxCallback(hcan, CAN_RX_FIFO1);
}
