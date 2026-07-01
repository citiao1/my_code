#include "imu.h"

uint8_t ahrsrx_buffer[256];
float YawEnableFlag = 0;
static AHRS_FEED imu_AHRS_feed = {0};

uint8_t *GiveAHRSBuffer(void)
{
	return ahrsrx_buffer;

}
AHRS_FEED *GetAHSRFeed(void)
{

	return &imu_AHRS_feed;
}
//整型转浮点类型
static float DataCharToFloat(uint8_t Data1,uint8_t Data2,uint8_t Data3,uint8_t Data4)
{
	static long long transition_32;//代转换的浮点
	static float tmp = 0;
	static float last_tmp = 0;
	static int sign = 0;
	static int exponent = 0;
	static float mantissa = 0;
	
	transition_32 = 0;
	transition_32 |= Data4 << 24;
	transition_32 |= Data3 << 16;
	transition_32 |= Data2 <<8;
	transition_32 |= Data1;
	
	sign = (transition_32 & 0x80000000) ? -1 : 1;
	
	//30-23 对应的？？
	exponent = ((transition_32 >> 23) & 0xff) - 127;
	//22-0 对应的？？
	mantissa = 1 + ((float)(transition_32 & 0x7fffff) / 0x7fffff);
	tmp = sign * mantissa * pow(2,exponent);
	last_tmp = tmp;
	tmp = last_tmp * 1/3 + tmp * 2/3;
	return tmp;
	
}

AHRS_FEED *AHRSPackHandle(uint8_t buff[])
{
	
	imu_AHRS_feed.LastYawDegree = imu_AHRS_feed.YawDegree;
	imu_AHRS_feed.RollSpeed = DataCharToFloat(buff[7],buff[8],buff[9],buff[10]);
	imu_AHRS_feed.PitchSpeed = DataCharToFloat(buff[11],buff[12],buff[13],buff[14]);
	imu_AHRS_feed.HeadingSpeed = DataCharToFloat(buff[15],buff[16],buff[17],buff[18]);
	imu_AHRS_feed.Roll = DataCharToFloat(buff[19], buff[20], buff[21], buff[22]);
	imu_AHRS_feed.Pitch = DataCharToFloat(buff[23], buff[24], buff[25], buff[26]);
	imu_AHRS_feed.Heading = DataCharToFloat(buff[27], buff[28], buff[29], buff[30]);
	imu_AHRS_feed.Q1 = DataCharToFloat(buff[31], buff[32], buff[33], buff[34]);
	imu_AHRS_feed.Q2 = DataCharToFloat(buff[35], buff[36], buff[37], buff[38]);
	imu_AHRS_feed.Q3 = DataCharToFloat(buff[39], buff[40], buff[41], buff[42]);
	imu_AHRS_feed.Q4 = DataCharToFloat(buff[43], buff[44], buff[45], buff[46]);
	imu_AHRS_feed.Timestamp = DataCharToFloat(buff[47], buff[48], buff[49], buff[50]);
	imu_AHRS_feed.PitchDegree = - (imu_AHRS_feed.Pitch * RAD_2_DEGERR);
	imu_AHRS_feed.YawDegree = imu_AHRS_feed.Heading * RAD_2_DEGERR;
	
	if (imu_AHRS_feed.YawDegree - imu_AHRS_feed.LastYawDegree <= -180.0f)
	{
		imu_AHRS_feed.yawrount++;
	}
	else if (imu_AHRS_feed.YawDegree - imu_AHRS_feed.LastYawDegree >= 180.0f)
	{
		imu_AHRS_feed.yawrount--;
	}
	imu_AHRS_feed.YawTotalDegree = 360.f * imu_AHRS_feed.yawrount + imu_AHRS_feed.YawDegree;
	
//	imu_AHRS_feed.YawTotalDegree = normalize_angle(imu_AHRS_feed.YawDegree);
	if(YawEnableFlag == 0)
	{
		YawEnableFlag = 1;
	}
	return &imu_AHRS_feed;
}

float normalize_angle(float angle)
{
    // 将角度标准化到0-360度范围
    angle = fmodf(angle, 360.0f);
    if (angle < 0) 
	{
        angle += 360.0f;
    }
    return angle;
}
