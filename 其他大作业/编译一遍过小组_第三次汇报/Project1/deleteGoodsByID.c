#include<stdio.h>
#include<string.h>
#include"establish.h"
#include"deleteGoodsByID.h"

void deleteGoodsByID() {
    char targetId[10];
    system("cls");
    printf("请输入要删除的商品ID:");
    scanf("%s", targetId);

    FILE* fp = fopen("goods", "rb");
    if (fp == NULL) {
        printf("无法打开商品文件 goods!\n");
        return;
    }

    
    FILE* tempFp = fopen("temp_goods", "wb");
    if (tempFp == NULL) {
        printf("创建临时文件失败!\n");
        fclose(fp);
        return;
    }

    struct item tempgoods;
    int found = 0;
    int remainingCount = 0;

    // 读取所有商品，将不匹配的写入临时文件
    while (fread(&tempgoods, sizeof(struct item), 1, fp) == 1) {
        if (strcmp(tempgoods.id, targetId) == 0) {
            found = 1;
            printf("已找到并删除以下商品:\n");
            printf("ID:%s | 品名:%s | 进价:%.2f | 售价:%.2f | 库存:%d\n",
                tempgoods.id, tempgoods.brand, tempgoods.in_price,
                tempgoods.out_price, tempgoods.storage);
        }
        else {
            fwrite(&tempgoods, sizeof(struct item), 1, tempFp);
            remainingCount++;
        }
    }

    fclose(fp);
    fclose(tempFp);

    if (!found) {
        printf("未找到ID为%s的商品!\n", targetId);
        remove("temp_goods"); // 删除临时文件
        return;
    }

    // 删除原文件并将临时文件重命名
    remove("goods");
    if (rename("temp_goods", "goods") != 0) {
        printf("删除商品失败!\n");
        return;
    }

    printf("商品删除成功! 剩余商品数量: %d\n", remainingCount);
}