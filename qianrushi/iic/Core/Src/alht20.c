#include "aht20.h"
#define AHT20_ADDRESS 0X70
u8 readBuffer[6]={0};
void AHT20_Init(void) {
    u8 readBuffer;
    HAL_Delay(40);
    HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, &readBuffer, 1, HAL_MAX_DELAY);
    if ((readBuffer&0x08)==0x00) {
        u8 sendBuffer[3]={0xBE,0x08,0x00};
        HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, sendBuffer, 3, HAL_MAX_DELAY);

    }
}
void AHT20_Read(float *Temperature,float *Humidity) {
    u8 sendBuffer[3]={0xAC,0x33,0x00};
    u8 readBuffer[6]={0};
    HAL_I2C_Master_Transmit_IT(&hi2c1, AHT20_ADDRESS, sendBuffer, 3);
    HAL_Delay(75);
    HAL_I2C_Master_Receive_IT(&hi2c1, AHT20_ADDRESS, readBuffer, 6);
    if ((readBuffer[0]&0x80)==0x00) {
        uint32_t data=0;
        data=((u32)readBuffer[3]>>4)+((u32)readBuffer[2]<<4)+((u32)readBuffer[1]<<12);
        *Humidity=data*100.0f/(1<<20);
        data=(((u32)readBuffer[3]&0x0F)<<16)+((u32)readBuffer[4]<<8)+(u32)readBuffer[5];
        *Temperature=data*200.0f/(1<<20)-50;
    }
}
void AHT20_Measure() {
    static u8 sendBuffer[3]={0xAC,0x33,0x00};
    HAL_I2C_Master_Transmit_DMA(&hi2c1, AHT20_ADDRESS, sendBuffer, 3);
}
void AHT20_Get() {
    HAL_I2C_Master_Receive_DMA(&hi2c1, AHT20_ADDRESS, readBuffer, 6);
}
void AHT20_Analysis(float *Temperature,float *Humidity) {
    if ((readBuffer[0]&0x80)==0x00) {
        uint32_t data=0;
        data=((u32)readBuffer[3]>>4)+((u32)readBuffer[2]<<4)+((u32)readBuffer[1]<<12);
        *Humidity=data*100.0f/(1<<20);
        data=(((u32)readBuffer[3]&0x0F)<<16)+((u32)readBuffer[4]<<8)+(u32)readBuffer[5];
        *Temperature=data*200.0f/(1<<20)-50;
    }
}