
#ifndef __ORDER_VIEW_H_
#define __ORDER_VIEW_H_

void viewHistoryOrders();
void calculateProductQuantities(Order* orders, const char* username, CategoryProfit** stats, int* count);
#endif