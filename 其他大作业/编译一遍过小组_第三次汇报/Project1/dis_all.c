#include <stdio.h>
#include "dis_all.h"
#include "establish.h"
#include "stock_warning.h" 
#include <windows.h>
#include "profit_chart_gdi.h"
extern struct item goods[];
void dis_all(int showChartOption) {
	system("cls");
	FILE* fp;
	struct item temp;
	fp = fopen("goods", "rb");
    int goodsCount = 0;
    
    while (fread(&goods[goodsCount], sizeof(struct item), 1, fp) == 1) {
        goodsCount++;  // 每读取一个商品，计数+1
    }
    fseek(fp, 0, SEEK_SET);
    printf("===================================== 商品列表 =====================================\n");
    printf("ID\t名称\t\t售价\t\t库存\t状态\n");
    printf("--------------------------------------------------------------------------------------\n");
    // 读取文件时检查 fread 返回值，确保 temp 被初始化
    while (fread(&temp, sizeof(struct item), 1, fp) == 1) { 
        printf("%s\t%s\t\t%.2f\t\t%d\t", temp.id, temp.brand, temp.out_price, temp.storage);
        if (temp.storage < 500 && temp.storage >= 0) {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
            printf("库存不足\n");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else {
            printf("库存正常\n");
        }
        
    }

    
    
    if (showChartOption) {
        char choice;
        printf("是否显示商品库存柱状图(Y/N): ");
        scanf(" %c", &choice);
        if (choice == 'Y' || choice == 'y') {
            if (goodsCount > 0) {
                showGoodsStockChart(goods, goodsCount, L"商品库存柱状图（颜色为预警色）");
            }
            else {
                printf("无商品数据可显示图表\n");
            }
        }
    }
    fclose(fp);
}
