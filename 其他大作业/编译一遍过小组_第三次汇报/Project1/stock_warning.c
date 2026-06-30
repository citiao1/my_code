#include <stdio.h>
#include <windows.h>  
#include "stock_warning.h"

// 单个商品库存预警检查
void checkSingleStockWarning(struct item* goods) {
    if (goods->storage < 10 && goods->storage >= 0) {
        // 设置控制台文本为红色以突出预警
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("[库存预警] 商品ID: %s, 名称: %s, 当前库存: %d (低于10，请及时补货)\n",
            goods->id, goods->brand, goods->storage);
        // 恢复默认文本颜色
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

// 检查所有商品库存
void checkAllStockWarnings() {
    FILE* fp = fopen("goods", "r");
    if (fp == NULL) {
        printf("商品文件打开失败，无法检查库存\n");
        return;
    }

    struct item tempGoods;
    int hasWarning = 0;  

    printf("\n===== 库存预警检查结果 =====\n");
    while (fread(&tempGoods, sizeof(struct item), 1, fp) == 1) {
        if (tempGoods.storage < 500 && tempGoods.storage >= 0) {
            checkSingleStockWarning(&tempGoods);
            hasWarning = 1;
        }
    }   

    if (!hasWarning) {
        printf("所有商品库存正常\n");
    }
    printf("===========================\n");

    fclose(fp);
}