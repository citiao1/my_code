#ifndef VEHICLE_COMM_H
#define VEHICLE_COMM_H

#include "usart.h"

void VehicleComm_Init(void);
void VehicleComm_Process(void);
void VehicleComm_SendTelemetry(uint32_t now);
void VehicleComm_SendStatus(uint32_t now);
uint8_t VehicleComm_IsTxBusy(void);
void VehicleComm_RxCallback(UART_HandleTypeDef *huart);
void VehicleComm_TxCallback(UART_HandleTypeDef *huart);
void VehicleComm_ErrorCallback(UART_HandleTypeDef *huart);

#endif
