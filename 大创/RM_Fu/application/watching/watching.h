#ifndef __WATCHING_H
#define __WATCHING_H

#include "main.h"
#include "motor.h"
#include "gimbal.h"
#include "IMU.h"
#include "usart.h"

#define DEGREE_2_RAD 0.0174529252f

//用联合体将无符号整形与其他类型地址联合起来赋值,避免类型转换时的错误
typedef union
{
    uint8_t color;
}color_u;

typedef union
{
    uint8_t pitch_measure_ar[4];
    float pitch_measure;
}pitch_measure_u;

typedef union
{
    uint8_t yaw_measure_ar[4];
    float yaw_measure;
}yaw_measure_u;

typedef union
{
    uint8_t fire_cmd;
}fire_cmd_u;

typedef union
{
    uint8_t yaw_ref_ar[4];
    float yaw_ref;
}yaw_ref_u;

typedef union
{
    uint8_t pitch_ref_ar[4];
    float pitch_ref;
}pitch_ref_u;

typedef union
{
    uint8_t distance_measure_ar[4];
    float distance_measure;
}distance_measure_u;

//发送给视觉16位数据第一位为0xFF,3_6位为yaw,7_10位为pitch,第11位为留空,第12位为留空,13位为0xfe14到16位留空
typedef struct
{
    uint8_t color;
    float pitch_measure;
    float yaw_measure;
}WatchingTransprot_t;


//一共接收视觉16字节数据第一字节为0xFF,15位留空,16位为0xFE.
typedef struct 
{
    uint8_t fire_cmd;                       //第2字节
    float yaw_ref;                          //第3_6字节
    float pitch_ref;                        //7_10字节
    float distance_measure;                 //11_14字节
    AHRS_FEED *Ahrs_feed;
}WatchingRecive_t;

void WatchingInit();
void WatchingRec(uint8_t *wrevbuff);
void WatchingTra();
uint8_t *GetWatchingRevBufff();
WatchingRecive_t *GetWatchingRev();

#endif
