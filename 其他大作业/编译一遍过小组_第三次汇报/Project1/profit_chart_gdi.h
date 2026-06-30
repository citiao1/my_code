#ifndef __PROFIT_CHART_GDI_H_
#define __PROFIT_CHART_GDI_H_

#include "profit_analysis.h" 
#include "establish.h" 
void showGoodsStockChart(struct item* goods, int count, const wchar_t* windowTitle);

void showProfitChart(CategoryProfit* profits, int count, const wchar_t* windowTitle);


void showMonthlyProfitChart(MonthlyProfit* profits, int count);

void showProfitPieChart(CategoryProfit* profits, int count, const wchar_t* windowTitle);

void showMonthlyLineChart(MonthlyProfit* profits, int count, const wchar_t* windowTitle);
#endif