#include "filter.h"
#include <string.h>
#include <math.h>

float Filter_Median(float *data, int len)
{
    float buf[10];
    if(len > 10) len = 10;   // 防止越界

    memcpy(buf, data, len * sizeof(float));

    for(int i = 0; i < len - 1; i++)
        for(int j = 0; j < len - i - 1; j++)
            if(buf[j] > buf[j+1]){
                float t = buf[j];
                buf[j] = buf[j+1];
                buf[j+1] = t;
            }

    return buf[len/2];
}

float Filter_Limit(float new_val, float old_val, float th)
{
    if(fabs(new_val-old_val)>th) return old_val;
    return new_val;
}

float Filter_LowPass(float new_val, float old_val, float alpha)
{
    return alpha * new_val + (1 - alpha) * old_val;
}
