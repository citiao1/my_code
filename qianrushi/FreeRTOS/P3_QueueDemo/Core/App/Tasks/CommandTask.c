#include "cmsis_os2.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "Types/LEDType.h"
#include "main.h"
void StartCommandTask(void *argument) {
    UART2_Receive_Start();
    uint8_t receive;
    uint8_t command[30];
    uint8_t commandIndex=0;
    uint8_t commandLength=0;
    for (;;) {
        osMessageQueueGet(CommandQueueHandle,&receive,0,osWaitForever);\
        if (commandIndex==0) {
            if (receive == 0xAA) {
                command[commandIndex++]=receive;
            }
        }else if (commandIndex==1) {
            if (receive<4||receive>sizeof(command)) {
                commandIndex=0;
                continue;
            }
            commandLength=receive;
            command[commandIndex++]=receive;
        }else {
            command[commandIndex++]=receive;
            if (commandIndex==commandLength) {
                uint8_t checksum=0;
                for (uint8_t i=0;i<commandLength-1;i++) {
                    checksum+=command[i];
                }
                if (checksum==command[commandLength-1]) {
                    for (uint8_t i=2;i<commandLength-2;i+=2) {
                        LEDMessage* message=pvPortMalloc(sizeof(LEDMessage));
                        message->color=command[i]-1;
                        message->state=command[i+1];
                        osMessageQueuePut(LEDQueueHandle,&message,0,osWaitForever);
                    }
                }
                commandIndex=0;
                commandLength=0;
            }
        }
    }
}
