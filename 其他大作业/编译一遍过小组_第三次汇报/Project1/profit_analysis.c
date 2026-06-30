#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "profit_analysis.h"
#include "user.h"  
#include "profit_chart_gdi.h"
#include "menu_utils.h"
#include "dis_all.h"
static Order* orderList = NULL;

// 生成唯一订单ID（格式：ORD+年月日时分秒）
void generateOrderId(char* orderId) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    sprintf(orderId, "ORD%d%02d%02d%02d%02d%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
}
static void freeOrders() {
    Order* currentOrder = orderList;
    while (currentOrder != NULL) {
        Order* nextOrder = currentOrder->next;
        // 释放当前订单的商品项链表
        OrderItem* currentItem = currentOrder->items;
        while (currentItem != NULL) {
            OrderItem* nextItem = currentItem->next;
            free(currentItem);
            currentItem = nextItem;
        }

        free(currentOrder);
        currentOrder = nextOrder;
    }
    orderList = NULL;
}

// 从文件加载订单数据
void loadOrders() {
    
    freeOrders();

    FILE* file = fopen("orders.dat", "rb");
    if (!file) {
        file = fopen("orders.dat", "wb");
        fclose(file);
        return;
    }

    Order* order;
    Order* lastOrder = NULL;
    OrderItem* item;
    OrderItem* lastItem;
    int itemCount;

    while (1) {
        order = (Order*)malloc(sizeof(Order));
        // 读取订单基本信息
        if (fread(order, sizeof(Order), 1, file) != 1) {
            free(order);
            break;
        }

        // 读取商品项数量
        if (fread(&itemCount, sizeof(int), 1, file) != 1) {
            free(order);
            break;
        }

        order->items = NULL;
        lastItem = NULL;

        // 读取所有商品项
        for (int i = 0; i < itemCount; i++) {
            item = (OrderItem*)malloc(sizeof(OrderItem));
            if (fread(item, sizeof(OrderItem), 1, file) != 1) {
                free(item);
                break;
            }
            item->next = NULL;
            if (!order->items) {
                order->items = item;
                lastItem = item;
            }
            else {
                lastItem->next = item;
                lastItem = item;
            }
        }

        // 加入订单链表
        order->next = NULL;
        if (!orderList) {
            orderList = order;
            lastOrder = order; 
        }
        else {
            if (lastOrder) { 
                lastOrder->next = order;
                lastOrder = order;
            }
            else {
                
                lastOrder = orderList;
                while (lastOrder->next) {
                    lastOrder = lastOrder->next;
                }
                lastOrder->next = order;
                lastOrder = order;
            }
        }
    }
    fclose(file);
}

// 保存新订单到文件
void saveOrder(Order* order) {
    FILE* file = fopen("orders.dat", "ab");
    if (!file) {
        printf("订单保存失败\n");
        return;
    }
    fwrite(order, sizeof(Order), 1, file);

    // 计算并写入订单项数量
    int itemCount = 0;
    OrderItem* item = order->items;
    while (item) {
        itemCount++;
        item = item->next;
    }
    fwrite(&itemCount, sizeof(int), 1, file);

    // 写入所有订单项
    item = order->items;
    while (item) {
        fwrite(item, sizeof(OrderItem), 1, file);
        item = item->next;
    }

    fclose(file);
}

// 解析日期字符串（YYYY-MM-DD）为time_t类型
time_t parseDate(const char* dateStr) {
    struct tm tm = { 0 };
    if (sscanf(dateStr, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3) {
        return -1;  
    }
    tm.tm_year -= 1900;  // 转换为1900年起的偏移量
    tm.tm_mon -= 1;      
    return mktime(&tm);
}

// 计算所有订单的总盈利
void calculateTotalProfit() {
    float total = 0;
    Order* order = orderList;
    while (order) {
        total += order->totalProfit;
        order = order->next;
    }
    printf("系统总盈利: %.2f 元\n", total);
}

// 按商品名称统计盈利
void calculateCategoryProfit() {
    
    typedef struct {
        char brand[20];
        float profit;
    } CategoryProfit;

    CategoryProfit* profits = NULL;
    int count = 0;

    // 遍历所有订单和订单项
    Order* order = orderList;
    while (order) {
        OrderItem* item = order->items;
        while (item) {
            int found = 0;
            
            for (int i = 0; i < count; i++) {
                if (strcmp(profits[i].brand, item->productBrand) == 0) {
                    profits[i].profit += item->profit;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                
                count++;
                profits = (CategoryProfit*)realloc(profits, count * sizeof(CategoryProfit));
                strcpy(profits[count - 1].brand, item->productBrand);
                profits[count - 1].profit = item->profit;
            }
            item = item->next;
        }
        order = order->next;
    }

    // 打印结果
    printf("按名称盈利分析:\n");
    for (int i = 0; i < count; i++) {
        printf("商品名称: %s, 总盈利: %.2f 元\n", profits[i].brand, profits[i].profit);
    }
    printf("是否导出数据到Excel? (Y/N): ");
    char choice;
    scanf(" %c", &choice);
    if (choice == 'Y' || choice == 'y') {
        char filename[100];
        printf("请输入导出文件名(例如: category_profit.csv): ");
        scanf("%s", filename);
        exportCategoryProfitToCSV(profits, count, filename);
    }
    showProfitChart(profits, count, L"产品利润图表");
    printf("是否显示饼状图? (Y/N): ");
    
    scanf(" %c", &choice);
    if (choice == 'Y' || choice == 'y') {
        showProfitPieChart(profits, count, L"商品利润饼状图");
    }
    free(profits);  

}

// 按时间范围统计盈利
void calculateTimeRangeProfit(const char* startDate, const char* endDate) {
    time_t start = parseDate(startDate);
    time_t end = parseDate(endDate);
    if (start == -1 || end == -1) {
        printf("日期格式错误！请使用YYYY-MM-DD\n");
        return;
    }

    // 结束日期设为当天23:59:59
    struct tm* tm = localtime(&end);
    tm->tm_hour = 23;
    tm->tm_min = 59;
    tm->tm_sec = 59;
    end = mktime(tm);

    // 计算范围内的总盈利
    MonthlyProfit* monthlyProfits = NULL;
    int count = 0;
    float total = 0;
    Order* order = orderList;
    while (order) {
        if (order->orderTime >= start && order->orderTime <= end) {
            total += order->totalProfit;

            // 提取订单的年月
            struct tm* orderTm = localtime(&order->orderTime);
            char monthStr[8];
            sprintf(monthStr, "%04d-%02d",
                orderTm->tm_year + 1900,
                orderTm->tm_mon + 1);

            // 检查是否已存在该月份的记录
            int found = 0;
            for (int i = 0; i < count; i++) {
                if (strcmp(monthlyProfits[i].month, monthStr) == 0) {
                    monthlyProfits[i].profit += order->totalProfit;
                    found = 1;
                    break;
                }
            }

            // 新月份，添加记录
            if (!found) {
                count++;
                monthlyProfits = (MonthlyProfit*)realloc(monthlyProfits,
                    count * sizeof(MonthlyProfit));
                strcpy(monthlyProfits[count - 1].month, monthStr);
                monthlyProfits[count - 1].profit = order->totalProfit;
            }
        }
        order = order->next;
    }

    printf("%s 至 %s 期间总利润: %.2f 元\n", startDate, endDate, total);

    // 显示月度利润图表
    if (count > 0) {
        printf("是否导出月度利润数据到Excel? (Y/N): ");
        char choice;
        scanf(" %c", &choice);
        if (choice == 'Y' || choice == 'y') {
            char filename[100];
            printf("请输入导出文件名(例如: monthly_profit.csv): ");
            scanf("%s", filename);
            exportMonthlyProfitToCSV(monthlyProfits, count, filename);
            showMonthlyProfitChart(monthlyProfits, count);
            printf("是否显示折线图? (Y/N): ");
            char choice;
            scanf(" %c", &choice);
            if (choice == 'Y' || choice == 'y') {
                showMonthlyLineChart(monthlyProfits, count, L"月度利润折线图");
            }
            free(monthlyProfits);
        }
        else {
            printf("该时间段内没有订单数据\n");
        }



    }
}
// 盈利分析菜单（交互入口）
void profitAnalysisMenu() {
    loadOrders();
    while (1) {
        char* options[] = {
            "查看总盈利",
            "按名称查看盈利",
            "按时间范围查看盈利",
            "特定商品按时间统计利润",
            "返回上一级"
        };
        char startDate[20], endDate[20];
        char productId[10];
        Menu* profitMenu = initMenu(options, 5);
        int choice = showMenu(profitMenu);
        freeMenu(profitMenu);

        switch (choice) {
        case 1:
            // 显示总利润逻辑
            system("cls");
            calculateTotalProfit();
            system("pause");
            break;
        case 2:
            // 显示商品利润明细
            system("cls");
            calculateCategoryProfit();
            system("pause");
            break;
        case 3:
            // 显示用户消费统计
            system("cls");
            printf("请输入开始日期(YYYY-MM-DD): ");
            fgets(startDate, sizeof(startDate), stdin);
            fgets(startDate, sizeof(startDate), stdin);
            startDate[strcspn(startDate, "\n")] = '\0';
            printf("请输入结束日期(YYYY-MM-DD): ");
            fgets(endDate, sizeof(endDate), stdin);
            endDate[strcspn(endDate, "\n")] = '\0';
            calculateTimeRangeProfit(startDate, endDate);
            system("pause");
            break;
        case 4:  // 特定商品统计功能
            system("cls");
            dis_all(0);
            printf("请输入要查询的商品ID: ");
            scanf("%s", productId);
            calculateSpecificProductProfitByTime(productId);
            system("pause");
            break;
        case 5:
            system("cls");
            return; 
        }
    }
}

void calculateSpecificProductProfitByTime(const char* productId) {
    
    loadOrders();
    Order* order = orderList;
    MonthlyProfit* monthlyProfits = NULL;
    int count = 0;

    while (order != NULL) {
        OrderItem* item = order->items;
        while (item != NULL) {
            // 匹配目标商品ID
            if (strcmp(item->productId, productId) == 0) {
                // 提取订单月份
                struct tm* orderTm = localtime(&order->orderTime);
                char monthStr[8];
                sprintf(monthStr, "%04d-%02d",
                    orderTm->tm_year + 1900,
                    orderTm->tm_mon + 1);

                // 累加对应月份利润
                int found = 0;
                for (int i = 0; i < count; i++) {
                    if (strcmp(monthlyProfits[i].month, monthStr) == 0) {
                        monthlyProfits[i].profit += item->profit;
                        found = 1;
                        break;
                    }
                }

                // 新增月份记录
                if (!found) {
                    count++;
                    monthlyProfits = (MonthlyProfit*)realloc(monthlyProfits,
                        count * sizeof(MonthlyProfit));
                    strcpy(monthlyProfits[count - 1].month, monthStr);
                    monthlyProfits[count - 1].profit = item->profit;
                }
            }
            item = item->next;
        }
        order = order->next;
    }

    // 显示统计结果
    if (count > 0) {
        printf("商品ID: %s 的月度利润统计:\n", productId);
        for (int i = 0; i < count; i++) {
            printf("%s: %.2f 元\n", monthlyProfits[i].month, monthlyProfits[i].profit);
        }
        // 显示月度利润图表
        showMonthlyLineChart(monthlyProfits, count, L"特定商品月度利润折线图");
        free(monthlyProfits);
    }
    else {
        printf("未找到商品ID: %s 的销售记录\n", productId);
    }
}
int exportCategoryProfitToCSV(CategoryProfit* profits, int count, const char* filename) {
    if (!profits || count <= 0 || !filename) return 1;

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("无法创建文件: %s\n", filename);
        return 1;
    }

    // 写入CSV表头
    fprintf(fp, "品牌,利润(元)\n");

    // 写入数据
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s,%.2f\n", profits[i].brand, profits[i].profit);
    }

    fclose(fp);
    printf("数据已成功导出到: %s\n", filename);
    return 0;
}
int exportMonthlyProfitToCSV(MonthlyProfit* profits, int count, const char* filename) {
    if (!profits || count <= 0 || !filename) return 1;

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("无法创建文件: %s\n", filename);
        return 1;
    }

    // 写入CSV表头
    fprintf(fp, "月份,利润(元)\n");

    // 写入数据
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s,%.2f\n", profits[i].month, profits[i].profit);
    }

    fclose(fp);
    printf("数据已成功导出到: %s\n", filename);
    return 0;
}
Order* getOrderList() {
    return orderList;  
}