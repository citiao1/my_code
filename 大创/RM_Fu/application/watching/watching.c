#include "watching.h"

/*
 *****************************************************************************************
 * 视觉通信模块（watching.c）
 *
 * UART6 通信协议：
 * - 发送（MCU -> 视觉）：16字节
 *   0xFF | color(1) | pitch(4) | yaw(4) | reserved(6) | 0xFE
 * - 接收（视觉 -> MCU）：16字节
 *   0xFF | fire_cmd(1) | yaw_ref(4) | pitch_ref(4) | distance(4) | 0xFE
 *
 * 功能：
 * 1. WatchingInit：绑定云台电机、IMU数据，初始化发送缓冲区头尾
 * 2. WatchingRec：在UART IDLE中断中调用，解析视觉反馈目标角度
 * 3. WatchingTra：周期发送当前云台姿态给视觉模块
 *****************************************************************************************
 */

uint8_t w_revbuff[16];                   // 接收缓冲区（DMA填充）
uint8_t w_trabuff[16] = {0};             // 发送缓冲区
WatchingRecive_t watching_recive;        // 视觉接收数据结构
WatchingTransprot_t watching_transprot;  // 视觉发送数据结构
color_u coloru;
pitch_measure_u pitch_measureu;
yaw_measure_u yaw_measureu;
fire_cmd_u fire_cmdu;
yaw_ref_u yaw_refu;
pitch_ref_u pitch_refu;
distance_measure_u distance_measureu;
MOTORInstance *yawmotor;
MOTORInstance *pitchmotor;
uint8_t *color;
AHRS_FEED *rad_feed;


/**
 * @brief  视觉模块初始化
 */
void WatchingInit(void)
{
    yawmotor = GetYawMotor();
    pitchmotor = GetPitchMotor();
    rad_feed = GetAHRSFeed();
    watching_recive.Ahrs_feed = rad_feed;
    watching_transprot.color = *color;
    watching_transprot.pitch_measure = rad_feed->PitchDegree;
    watching_transprot.yaw_measure = rad_feed->YawDegree;

    w_trabuff[0] = 0xFF;  // 帧头
    w_trabuff[12] = 0xFE; // 帧尾
}

/**
 * @brief  视觉接收解析（在UART IDLE中断中调用）
 */
void WatchingRec(uint8_t *wrevbuff)
{
    if(wrevbuff[0] == 0xFF && wrevbuff[15] == 0xFE)
    {   
        watching_recive.fire_cmd = wrevbuff[1];
        for(int i = 0;i < 4;i++)
        {
            yaw_refu.yaw_ref_ar[i] = wrevbuff[i + 2];
            pitch_refu.pitch_ref_ar[i] = wrevbuff[i + 6];
            distance_measureu.distance_measure_ar[i] = wrevbuff[i + 10];
        }
        watching_recive.yaw_ref = yaw_refu.yaw_ref * RAD_2_DEGREE + rad_feed->yawrount * 360.0f;  // 多圈角度修正
        watching_recive.pitch_ref = pitch_refu.pitch_ref * RAD_2_DEGREE;
        watching_recive.distance_measure = distance_measureu.distance_measure;
    }
}

/**
 * @brief  周期发送视觉数据（在WatchingTask中调用）
 */
void WatchingTra(void)
{
    w_trabuff[1] = *color;  // 当前识别颜色
    watching_transprot.pitch_measure = rad_feed->PitchDegree * DEGREE_2_RAD;
    watching_transprot.yaw_measure = rad_feed->YawDegree * DEGREE_2_RAD;
    pitch_measureu.pitch_measure = watching_transprot.pitch_measure;
    yaw_measureu.yaw_measure = watching_transprot.yaw_measure;
    for(int i = 0; i < 4;i++)
    {
        w_trabuff[i + 2] = pitch_measureu.pitch_measure_ar[i];
        w_trabuff[i + 6] = yaw_measureu.yaw_measure_ar[i];
    }
    HAL_UART_Transmit(&huart6, w_trabuff, sizeof(w_trabuff), 50);
    while(!(huart6.gState | HAL_UART_STATE_BUSY_TX));
}

/**
 * @brief  获取视觉接收缓冲区指针（供DMA填充）
 */
uint8_t *GetWatchingRevBufff(void)
{
    return w_revbuff;
}

/**
 * @brief  获取视觉接收结果
 */
WatchingRecive_t *GetWatchingRev(void)
{
    return &watching_recive;
}




