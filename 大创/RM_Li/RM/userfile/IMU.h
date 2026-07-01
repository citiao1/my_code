#ifndef _imu__H__
#define _imu__H__

#include "main.h"
#include "math.h"
#include "usart.h"
#include "stdlib.h"
#include "string.h"
#define RAD_2_DEGERR        180.0f/3.1415926536
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

float normalize_angle(float angle);
uint8_t *GiveAHRSBuffer(void);
AHRS_FEED *AHRSPackHandle(uint8_t buff[]);
AHRS_FEED *GetAHSRFeed(void);
#endif
