#ifndef __USER_CONSUMPTION_H_
#define __USER_CONSUMPTION_H_

#include "profit_analysis.h"  

// 用户累计消费结构体
typedef struct {
    char username[20];  // 用户名
    float total_spent;  // 累计消费金额
} UserConsumption;


void calculate_user_consumption(UserConsumption** consumptions, int* count);
void show_user_consumption_chart();

#endif