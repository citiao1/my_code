#ifndef IMU_H
#define IMU_H

#include "main.h"
#include "math.h"
#include "usart.h"
#include "memory.h"

#define PACKSTART           0xFC
#define AHRSPACK            0x41
#define AHRSDATASTART       7
#define AHRSDATAEND         54
#define AHRSPACKEND         55
#define PACKEND             0xFD
#define RAD_2_DEGERR        180.0f/3.1415926536
#define DEgeRR_2_RAD        3.1415926536/180.0f

//AHRS数据包

typedef enum
{
    AHRSWAITING = 0x000,
    AHRSRXING = 0x001,
    AHRSSUCCESS = 0x010
}IMU_RX_STATE_e;

typedef struct 
{

    float RollSpeed;
    float PitchSpeed;
    float HeadingSpeed;
    float Roll;
    float Pitch;
    float Heading;
    float Q1;
    float Q2;
    float Q3;
    float Q4;
    float YawDegree;
    float PitchDegree;
    float YawTotalDegree;
    float LastYawDegree;
    int yawrount;
    int64_t Timestamp;

}AHRS_FEED;

uint8_t *GetRxAHRBuff();
uint8_t *Getsgbuff();
AHRS_FEED *AHRSPackHandle(uint8_t buff[]);
AHRS_FEED *GetAHRSFeed();

#endif 