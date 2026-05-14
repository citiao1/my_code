#include "my_function.h"
#include "stdio.h"
#include "string.h"
#include "gpio.h"
#include "usart.h"
uint8_t receiveData[1000];
u32 uwTick_Key=0;
u8 KeyValue;
u8 KeyState;
unsigned char Rx_Buff[BUFFERSIZE];
unsigned char Rx_flg;
unsigned int  Rx_cnt;
uint8_t aRX;
uint8_t receive;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart2)
    {


        Rx_Buff[Rx_cnt++] = aRX;

        if(0x0a == aRX)
        {
            Rx_flg = 1;
        }

        HAL_UART_Receive_IT(&huart2, &aRX, REC_LENGTH);
    }
}
void Blue_LED(const char *state)
{
    if (strcmp(state,"ON")==0)HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_SET);
    if (strcmp(state,"OFF")==0)HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,GPIO_PIN_RESET);
}
void Red_LED(const char *state)
{
    if (strcmp(state,"ON")==0)HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET);
    if (strcmp(state,"OFF")==0)HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET);
}
void Green_LED(const char *state)
{
    if (strcmp(state,"ON")==0)HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_SET);
    if (strcmp(state,"OFF")==0)HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET);
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart==&huart3) {
        HAL_UART_Transmit_IT(&huart3,receiveData,Size);
        if (receiveData[0]==0xAA) {
            if (receiveData[1]==Size) {
                uint8_t sum=0;
                for (int i=0;i<Size-1;i++) {
                    sum+=receiveData[i];
                }
                if (sum==receiveData[Size-1]) {
                    for (int i=2;i<Size-1;i+=2) {
                        GPIO_PinState state=GPIO_PIN_SET;
                        if (receiveData[i+1]==0x00) {
                            state=GPIO_PIN_RESET;
                        }
                        if (receiveData[i]==0x01) {
                            HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,state);
                        }else if (receiveData[i]==0x02) {
                            HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,LED_GREEN_Pin,state);
                        }else if (receiveData[i]==0x03) {
                            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin,state);
                        }
                    }
                }
            }
        }


        HAL_UARTEx_ReceiveToIdle_IT(&huart3,receiveData,sizeof(receiveData));
    }
}
void Key_Pro(void) {
    if((uwTick-uwTick_Key)<10)return;
    uwTick_Key=uwTick;
    KeyValue=key_scanf();
    if(KeyValue==1){
        KeyState=1;
    }else if(KeyValue==2) {
        KeyState=2;
    }
    if (KeyState==1) {
        Blue_LED(ON);
        KeyState=0;
    }else if(KeyState==2) {
        Blue_LED(OFF);
        KeyState=0;
    }
}
void Rx_Process(void)
{
    if(Rx_flg ==1)
    {
        //if((!strcmp((char *)Rx_Buff,"ChargeCycles0\r\n"))){

        HAL_UART_Transmit(&huart2, (u8*)Rx_Buff, strlen(Rx_Buff), 0xffff);
        // }



        memset(Rx_Buff,0x00, 256);
        Rx_cnt =0;
        Rx_flg =0;
    }
}
uint8_t key_scanf(void)
{
    uint8_t key_return = 0;
    static uint8_t key_state;
    switch(key_state)
    {
        case 0:
            if(B1 && B2 ){
                key_state = KEY_STATE_0;
            }
            else{
                key_state = KEY_STATE_1;
            }
            break;
        case 1:
            if(B1 && B2 ){
                key_state = KEY_STATE_0;
            }
            else{
                if(!B1) key_return = 1;
                else if(!B2) key_return = 2;

                key_state = KEY_STATE_2;
            }
            break;
        case 2:
            if(B1 && B2 ){
                key_state = KEY_STATE_0;
            }
            break;
        default:
            break;
    }

    return key_return;
}