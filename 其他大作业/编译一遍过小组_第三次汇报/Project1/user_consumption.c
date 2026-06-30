#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_consumption.h"
#include "profit_analysis.h"
#include "profit_chart_gdi.h"

// 计算所有用户的累计消费
void calculate_user_consumption(UserConsumption** consumptions, int* count) {
    *count = 0;
    *consumptions = NULL;

    // 从现有订单列表获取数据
    loadOrders();  
    Order* order = getOrderList();  // 获取订单列表

    while (order != NULL) {
        int found = 0;
        // 查找用户是否已在统计列表中
        for (int i = 0; i < *count; i++) {
            if (strcmp((*consumptions)[i].username, order->username) == 0) {
                // 累加该用户的消费金额（计算订单总金额）
                float order_total = 0;
                OrderItem* item = order->items;
                while (item != NULL) {
                    order_total += item->outPrice * item->quantity;
                    item = item->next;
                }
                (*consumptions)[i].total_spent += order_total;
                found = 1;
                break;
            }
        }

        // 新用户，添加到统计列表
        if (!found) {
            (*count)++;
            UserConsumption* temp = (UserConsumption*)realloc(
                *consumptions, *count * sizeof(UserConsumption)
            );
            if (temp == NULL) {
                printf("内存分配失败\n");
                (*count)--;
                free(*consumptions);
                *consumptions = NULL;
                return;
            }
            *consumptions = temp;

            // 计算当前订单总金额
            float order_total = 0;
            OrderItem* item = order->items;
            while (item != NULL) {
                order_total += item->outPrice * item->quantity;
                item = item->next;
            }

            strncpy((*consumptions)[*count - 1].username, order->username, 19);
            (*consumptions)[*count - 1].username[19] = '\0';
            (*consumptions)[*count - 1].total_spent = order_total;
        }

        order = order->next;
    }
}

// 显示用户累计消费柱状图
void show_user_consumption_chart() {
    UserConsumption* consumptions = NULL;
    int count = 0;
    calculate_user_consumption(&consumptions, &count);

    if (count <= 0 || consumptions == NULL) {
        printf("没有用户消费数据可显示\n");
        return;
    }

    // 转换为图表所需格式
    CategoryProfit* chart_data = (CategoryProfit*)malloc(count * sizeof(CategoryProfit));
    if (chart_data == NULL) {
        printf("内存分配失败\n");
        free(consumptions);
        return;
    }

    // 填充图表数据并在控制台显示
    for (int i = 0; i < count; i++) {
        strncpy(chart_data[i].brand, consumptions[i].username, 19);
        chart_data[i].brand[19] = '\0';
        chart_data[i].profit = consumptions[i].total_spent;
        printf("用户: %s, 累计消费: %.2f 元\n",
            consumptions[i].username,
            consumptions[i].total_spent);
    }

    
    showProfitChart(chart_data, count, L"用户累计消费柱状图");

    // 释放内存
    free(consumptions);
    free(chart_data);
}