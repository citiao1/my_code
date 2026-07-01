#include "IMU.h"

/*
 *****************************************************************************************
 * IMU 数据解析模块
 *
 * AHRS数据包格式（以Bytes为单位）：
 * [0]  0x?? (上位机协议头)
 * [7~10]  RollSpeed (float)
 * [11~14] PitchSpeed
 * ...
 * 共计48字节浮点数据，最后有时间戳
 *
 * 解析内容：
 * - 姿态角/角速度（Roll/Pitch/Heading）
 * - 四元数（Q1~Q4）
 * - 时间戳
 * - Yaw累计角（多圈处理）
 *****************************************************************************************
 */

static uint8_t rx_AHRS_buff[256];   // IMU接收缓冲区（DMA）
static uint8_t rxsgbuff;            // 备用（未使用）
static AHRS_FEED imu_AHRS_feed = {0};
static uint32_t Atick;
float yawcotor = 0;                 // 云台初始化同步标志


/**
 * @brief  获取IMU接收缓冲区指针
 */
uint8_t *GetRxAHRBuff(void)
{
    return rx_AHRS_buff;
}

uint8_t *Getsgbuff(void)
{
    return &rxsgbuff;
}

/**
 * @brief  将4字节转换为浮点数（IEEE754）
 */
static float DATA_Trans(uint8_t Data_1, uint8_t Data_2, uint8_t Data_3, uint8_t Data_4)
{
    static long long transition_32;
    static float tmp = 0;
    static float last_tmp = 0;
    static int sign = 0;
    static int exponent = 0;
    static float mantissa = 0;
    transition_32 = 0;
    transition_32 |= Data_4 << 24; // 得到数据的底 8 位
    transition_32 |= Data_3 << 16;
    transition_32 |= Data_2 << 8;
    transition_32 |= Data_1;                      // 得到数据的高 8 位
    sign = (transition_32 & 0x80000000) ? -1 : 1; // 符号位
    // 先右移操作，再按位与计算，出来结果是 30 到 23 位对应的 e
    exponent = ((transition_32 >> 23) & 0xff) - 127;
    // 将 22~0 转化为 10 进制，得到对应的 x 系数
    mantissa = 1 + ((float)(transition_32 & 0x7fffff) / 0x7fffff);
    tmp = sign * mantissa * pow(2, exponent);
    last_tmp = tmp;
    tmp = last_tmp*1/3 + tmp*2/3;
    return tmp;
}

/**
 * @brief  解析AHRS数据包
 * @param  buff: 指向IMU数据缓冲区的指针
 * @retval AHRS_FEED*: 解析后的姿态结构体
 */
AHRS_FEED *AHRSPackHandle(uint8_t buff[])
{
    Atick = uwTick;
        imu_AHRS_feed.LastYawDegree = imu_AHRS_feed.YawDegree;
        imu_AHRS_feed.RollSpeed = DATA_Trans(buff[7],buff[8],buff[9],buff[10]);
        imu_AHRS_feed.PitchSpeed = DATA_Trans(buff[11],buff[12],buff[13],buff[14]);
        imu_AHRS_feed.HeadingSpeed = DATA_Trans(buff[15],buff[16],buff[17],buff[18]);
        imu_AHRS_feed.Roll = DATA_Trans(buff[19], buff[20], buff[21], buff[22]);
        imu_AHRS_feed.Pitch = DATA_Trans(buff[23], buff[24], buff[25], buff[26]);
        imu_AHRS_feed.Heading = DATA_Trans(buff[27], buff[28], buff[29], buff[30]);
        imu_AHRS_feed.Q1 = DATA_Trans(buff[31], buff[32], buff[33], buff[34]);
        imu_AHRS_feed.Q2 = DATA_Trans(buff[35], buff[36], buff[37], buff[38]);
        imu_AHRS_feed.Q3 = DATA_Trans(buff[39], buff[40], buff[41], buff[42]);
        imu_AHRS_feed.Q4 = DATA_Trans(buff[43], buff[44], buff[45], buff[46]);
        imu_AHRS_feed.Timestamp = DATA_Trans(buff[47], buff[48], buff[49], buff[50]);
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
    // }
    // else 
    // {
    //     imu_AHRS_feed = imu_AHRS_feed;
    // }
    if(yawcotor == 0)
        yawcotor = 1;
    return &imu_AHRS_feed;
}

AHRS_FEED *GetAHRSFeed(void)
{
    return &imu_AHRS_feed;
}




