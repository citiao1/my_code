#ifndef __PROFIT_ANALYSIS_H_
#define __PROFIT_ANALYSIS_H_

#include "establish.h"  
#include <time.h>     

typedef struct {
    char brand[20];  // 商品名称
    float profit;    // 总利润
} CategoryProfit;
typedef struct {
    char month[8];  // 月份格式 "YYYY-MM"
    float profit;   // 月度利润
} MonthlyProfit;
// 订单项结构体
typedef struct OrderItem {
    char productId[10];   // 商品ID
    char productBrand[20];// 商品名称
    int quantity;         // 购买数量
    float inPrice;        // 进货价
    float outPrice;       // 售价
    float profit;         // 单项利润 
    struct OrderItem* next;
} OrderItem;

// 订单结构体
typedef struct Order {
    char orderId[30];     // 订单唯一ID
    char username[20];    // 下单用户
    time_t orderTime;     // 下单时间
    float totalProfit;    // 订单总利润
    OrderItem* items;     // 订单包含的商品项
    struct Order* next;   // 链表指针
} Order;


void loadOrders();


void saveOrder(Order* order);


void calculateTotalProfit();


void calculateCategoryProfit();


void calculateTimeRangeProfit(const char* startDate, const char* endDate);


void profitAnalysisMenu();
void calculateSpecificProductProfitByTime(const char* productId);
int exportCategoryProfitToCSV(CategoryProfit* profits, int count, const char* filename);
int exportMonthlyProfitToCSV(MonthlyProfit* profits, int count, const char* filename);
Order* getOrderList();  

#endif