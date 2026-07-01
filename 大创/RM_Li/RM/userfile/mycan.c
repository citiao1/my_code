#include "can.h"
#include "stm32f4xx_it.h"
#include "mycan.h"
#include "stdlib.h"
#include "stdio.h"
MotorData motor_data1[9];
void MycanInit(void){
  
	   HAL_StatusTypeDef status;
    do {
        status = HAL_CAN_Start(&hcan1);  // 启动CAN
        if (status != HAL_OK) {
            HAL_Delay(100);  // 延迟后重试
        }
    } while (status != HAL_OK);
	
	CAN_FilterTypeDef filter_Init;
	filter_Init.FilterActivation=ENABLE;
	filter_Init.FilterBank=0;
	filter_Init.FilterFIFOAssignment=CAN_RX_FIFO0;
	filter_Init.FilterIdHigh=0x0000;
	filter_Init.FilterIdLow=0x0000;
	filter_Init.FilterMaskIdHigh=0x0000;
	filter_Init.FilterMaskIdLow=0x0000;
	filter_Init.FilterMode=CAN_FILTERMODE_IDMASK;
	filter_Init.FilterScale=CAN_FILTERSCALE_16BIT;
	filter_Init.SlaveStartFilterBank=14;
	
	HAL_CAN_ConfigFilter(&hcan1,&filter_Init);
	HAL_CAN_Start(&hcan1);
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); 
}

static void EcodeProc(MotorData *angleproc)
{
	angleproc->last_ecode = angleproc->ecode;
	angleproc->ecode = angleproc->angle;//得到电机编码器值
	angleproc->single_angle = angleproc->ecode * ECODE_K;
	
	float delta_angle = angleproc->single_angle - angleproc->last_singleangle;
	if(delta_angle > 180.0f)
	{
		delta_angle -=360.0f;
	}
	else if(delta_angle < -180.0f)
	{
		delta_angle +=360.0f;
	}
	angleproc->total_angle += delta_angle;
	angleproc->last_singleangle = angleproc->single_angle;
}
void MotorProcess(CAN_RxHeaderTypeDef *rx_header, uint8_t data[8], MotorData *pdata) {
    if (rx_header->StdId > 0x200 && rx_header->StdId <= 0x207) 
	{
        uint8_t motor_id1 = rx_header->StdId - 0x201;
        pdata[motor_id1].angle  = (data[0] << 8) | data[1];
		pdata[motor_id1].speed = (data[2] << 8) | data[3];
        pdata[motor_id1].current = (data[4] << 8) | data[5];
        pdata[motor_id1].temp   = data[6];
		
    }
	if (rx_header->StdId > 0x208 && rx_header->StdId <= 0x20A) 
	{
        uint8_t motor_id = rx_header->StdId - 0x202;
        pdata[motor_id].angle  = (data[0] << 8) | data[1];
		pdata[motor_id].speed = (data[2] << 8) | data[3];
        pdata[motor_id].current = (data[4] << 8) | data[5];
        pdata[motor_id].temp   = data[6];
		
    }
	EcodeProc(&pdata[7]);
	EcodeProc(&pdata[8]);
}   



MotorData *GetMotorData(void)
{
return motor_data1;
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
	{
    
	uint8_t data[8];
    CAN_RxHeaderTypeDef rx_header;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, data);
    MotorProcess(&rx_header, data, motor_data1); 
}



void CanMotorTransmit(uint16_t id,int16_t v1,int16_t v2,int16_t v3,int16_t v4)
{
	uint8_t tx_data[8]; 
	CAN_TxHeaderTypeDef tx_header;
	tx_header.IDE= CAN_ID_STD;
	tx_header.RTR=CAN_RTR_DATA;
	tx_header.StdId=id;
	tx_header.DLC=8;
	tx_data[0]=(v1>>8)&0xff;
	tx_data[1]=(v1)&0xff;
	tx_data[2]=(v2>>8)&0xff;;
	tx_data[3]=(v2)&0xff;;
	tx_data[4]=(v3>>8)&0xff;;	
	tx_data[5]=(v3)&0xff;;
	tx_data[6]=(v4>>8)&0xff;;
	tx_data[7]=(v4)&0xff;;
	HAL_CAN_AddTxMessage(&hcan1,&tx_header,tx_data,(uint32_t*)CAN_TX_MAILBOX0);
}
