#include<stdio.h>
#include "establish.h"
#include "dis_all.h"
int isIdExists(const char* id) {
    FILE* fp = fopen("goods", "rb");
    if (fp == NULL) {
        return 0; // 文件不存在，无重复ID
    }

    struct item temp;
    while (fread(&temp, sizeof(struct item), 1, fp) == 1) {
        if (strcmp(temp.id, id) == 0) {
            fclose(fp);
            return 1; // 存在重复ID
        }
    }
    fclose(fp);
    return 0;
}

void establish() {
    FILE* fp;
    int i = 0;
    struct item newGoods; // 用于临时存储单个新商品

    dis_all(0);
    printf("===== 新增商品信息 =====\n");
    printf("提示：输入商品名称为\"00\"时结束添加\n");
    printf("------------------------\n");

    
    fp = fopen("goods", "ab");
    if (fp == NULL) {
        printf("文件打开失败，无法添加商品\n");
        return;
    }

    while (1) {
        printf("\n商品名称:");
        fflush(stdin);
        scanf("%s", newGoods.brand);

        // 检查是否结束添加
        if (strcmp(newGoods.brand, "00") == 0) {
            break;
        }

        // 检查ID唯一性
        while (1) {
            printf("商品ID:");
            fflush(stdin);
            scanf("%s", newGoods.id);

            if (isIdExists(newGoods.id)) {
                printf("错误：该ID已存在，请重新输入\n");
            }
            else {
                break;
            }
        }

        printf("进货价:");
        fflush(stdin);
        scanf("%f", &newGoods.in_price);

        printf("售价:");
        fflush(stdin);
        scanf("%f", &newGoods.out_price);

        printf("库存数量:");
        fflush(stdin);
        scanf("%d", &newGoods.storage);

        // 写入新商品到文件
        fwrite(&newGoods, sizeof(struct item), 1, fp);
        i++;
        printf("已添加第%d个商品\n", i);
    }

    fclose(fp);
    printf("\n添加完成，共新增%d个商品\n", i);
    system("pause");
    system("cls");
}