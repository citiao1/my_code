#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "profit_analysis.h"
#include "user.h"
#include "profit_chart_gdi.h" 
void calculateProductQuantities(Order* orders, const char* username, CategoryProfit** stats, int* count) {
    *count = 0;
    *stats = NULL; 
    Order* order = orders;

    while (order != NULL) {
        if (strcmp(order->username, username) == 0) {
            OrderItem* item = order->items;
            while (item != NULL) {
                int found = 0;
                // 查找是否已存在该商品
                for (int i = 0; i < *count; i++) {
                    if (strcmp((*stats)[i].brand, item->productBrand) == 0) {
                        (*stats)[i].profit += item->quantity;  // 叠加数量
                        found = 1;
                        break;
                    }
                }
                // 新商品，添加到统计列表
                if (!found) {
                    (*count)++; 
                    CategoryProfit* temp = (CategoryProfit*)realloc(*stats, *count * sizeof(CategoryProfit));
                    if (temp == NULL) {
                        printf("内存分配失败，无法添加商品\n");
                        (*count)--; 
                        free(*stats);
                        *stats = NULL;
                        return;
                    }
                    *stats = temp;

                    if (*stats == NULL || *count <= 0) {
                        printf("统计数据异常，无法添加商品\n");
                        return;
                    }

                    if (item == NULL || item->productBrand == NULL) {
                        printf("商品信息无效，跳过\n");
                        (*count)--;
                        continue;
                    }

                    strncpy((*stats)[*count - 1].brand, item->productBrand, 19);
                    (*stats)[*count - 1].brand[19] = '\0';
                    (*stats)[*count - 1].profit = item->quantity;  
                }
                item = item->next;
            }
        }
        order = order->next;
    }
}


void viewHistoryOrders() {
    system("cls");
    printf("===== 我的历史订单 =====\n");
    printf("当前用户: %s\n", currentUser); 

    loadOrders();
    Order* order = getOrderList();
    int orderCount = 0;

    if (!order) {
        printf("没有任何订单数据\n"); 
        system("pause");
        system("cls");
        return;
    }

    while (order != NULL) {
        if (strcmp(order->username, currentUser) == 0) {
            orderCount++;
            // 格式化订单时间
            struct tm* orderTime = localtime(&order->orderTime);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", orderTime);

            // 显示订单基本信息
            printf("\n------------------------\n");
            printf("订单编号: %s\n", order->orderId);
            printf("下单时间: %s\n", timeStr);
            printf("订单商品:\n");
            printf("------------------------------------------------\n");
            printf("商品ID   商品名称   数量   单价   小计\n");

            // 显示订单中的商品
            OrderItem* item = order->items;
            while (item != NULL) {
                float subtotal = item->quantity * item->outPrice;
                printf("%-8s %-8s %-6d %.2f   %.2f\n",
                    item->productId,
                    item->productBrand,
                    item->quantity,
                    item->outPrice,
                    subtotal);
                item = item->next;
            }

            printf("------------------------------------------------\n");
            float totalAmount = 0;
            OrderItem* tempItem = order->items;  
            while (tempItem != NULL) {
                totalAmount += tempItem->outPrice * tempItem->quantity;  
                tempItem = tempItem->next;  
            }
            printf("订单总金额: %.2f 元\n", totalAmount);
        }
        order = order->next;
        

        
    }
    if (orderCount == 0) {
        printf("您暂无历史订单\n");
    }   
    printf("\n------------------------\n");
    char choice;
    printf("请选择要显示的图表类型:\n");
    printf("1 - 商品数量统计柱状图\n");
    printf("2 - 商品数量统计饼状图\n");
    printf("3 - 同时显示两种图表\n");
    printf("其他键 - 不显示图表\n");
    printf("请选择: ");
    scanf(" %c", &choice);

    CategoryProfit* productStats = NULL;
    int count = 0;
    calculateProductQuantities(getOrderList(), currentUser, &productStats, &count);

    if (count > 0) {
        switch (choice) {
        case '1':
            showProfitChart(productStats, count, L"商品数量统计柱状图");
            break;
        case '2':
            showProfitPieChart(productStats, count, L"商品数量统计饼状图");
            break;
        case '3':
            showProfitChart(productStats, count, L"商品数量统计柱状图");
            showProfitPieChart(productStats, count, L"商品数量统计饼状图");
            break;
        default:
            printf("不显示图表\n");
        }
        free(productStats);
    }
    else {
        printf("没有足够数据生成图表\n");
    }


    system("pause");
    system("cls");
}
