#ifndef LQ_CCU6_H
#define LQ_CCU6_H

#include "lq_include.h"

/**
 * 	CCU6模块枚举
 */
typedef enum
{
    CCU60,
    CCU61
} CCU6_t;

/**
 * 	CCU6通道枚举
 */
typedef enum
{
    CCU6_Channel0,
    CCU6_Channel1,
} CCU6_Channel_t;

/*************************************************************************
 *  函数名称：CCU6_InitConfig CCU6
 *  功能说明：定时器周期中断初始化
 *  参数说明：ccu6    ： ccu6模块            CCU60 、 CCU61
 *  参数说明：channel ： ccu6模块通道  CCU6_Channel0 、 CCU6_Channel1
 *  参数说明：us      ： ccu6模块  中断周期时间  单位us
 *  函数返回：无
 *  备    注：
 *************************************************************************/
void CCU6_InitConfig(CCU6_t ccu6, CCU6_Channel_t channel, unsigned long us);

void CCU6_DisableInterrupt(CCU6_t ccu6, CCU6_Channel_t channel);

void CCU6_EnableInterrupt(CCU6_t ccu6, CCU6_Channel_t channel);

#endif /* LQ_CCU6_H */