#ifndef ECODER_H
#define ECODER_H

#include "main.h"
#include "bsp_can.h"
#include "memory.h"
#include "string.h"
#include "can.h"


#define ENCODE_2_DEGREE     0.010986328125


//编码器实例
typedef struct 
{
    float *single_ecode;
    float *single_angle;
    float *total_angle;
    float *last_ecode;
    int *round;
    float *d_ecode;
    float *averaged_ecode;
    CANInstance *ecoder_can_instance;

}EcoderInstance;

EcoderInstance *EcoderInit();

#endif // !