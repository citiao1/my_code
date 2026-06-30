#include<stdio.h>
#include<string.h>
#include"establish.h"
#include"modifyGoodsByID.h"
#include "dis_all.h"
#include "stock_warning.h"

void modifyGoodsByID() {
    char targetId[10];
    char continueModifyCurrent = ' '; // 是否继续修改当前商品
    char continueModifyOther = ' ';   // 是否继续修改其他商品
    do {
        system("cls");
        dis_all(0);
        printf("\n");
        printf("请输入要修改的商品编号:");
        scanf("%s", targetId);
        FILE* fp = fopen("goods", "rb+");
        if (fp == NULL) {
            printf("无法打开文件 goods！\n");
            return;
        }


        struct item tempgoods;
        int found = 0;
        long filePos;


        while (1) {

            filePos = ftell(fp);

            if (fread(&tempgoods, sizeof(struct item), 1, fp) != 1) {
                break;
            }

            // 匹配目标商品ID
            if (strcmp(tempgoods.id, targetId) == 0) {
                found = 1;
                printf("\n找到商品：\n");
                printf("ID:%s | 品名:%s | 进价:%.2f | 售价:%.2f | 库存:%d\n",
                    tempgoods.id, tempgoods.brand, tempgoods.in_price,
                    tempgoods.out_price, tempgoods.storage);

                // 修改商品信息
                do {
                    printf("请选择需要更改的信息：\n");
                    printf("1.商品名称：\n");
                    printf("2.进价：\n");
                    printf("3.售价：\n");
                    printf("4.库存数量：\n");
                    printf("5.全部商品信息：\n");
                    int choice2;
                    scanf("%d", &choice2);
                    getchar();
                    switch (choice2) {
                    case 1:
                        printf("请输入新品名:");
                        getchar();
                        fgets(tempgoods.brand, sizeof(tempgoods.brand), stdin);
                        tempgoods.brand[strcspn(tempgoods.brand, "\n")] = '\0';
                        break;
                    case 2:

                        printf("请输入新进价：");
                        scanf("%f", &tempgoods.in_price);
                        break;
                    case 3:

                        printf("请输入新售价:");
                        scanf("%f", &tempgoods.out_price);
                        break;
                    case 4:
                        printf("请输入新库存:");
                        scanf("%d", &tempgoods.storage);
                        break;
                    case 5:
                        printf("请输入新品名:");
                        getchar();
                        fgets(tempgoods.brand, sizeof(tempgoods.brand), stdin);
                        tempgoods.brand[strcspn(tempgoods.brand, "\n")] = '\0';

                        printf("请输入新进价：");
                        scanf("%f", &tempgoods.in_price);

                        printf("请输入新售价:");
                        scanf("%f", &tempgoods.out_price);

                        printf("请输入新库存:");
                        scanf("%d", &tempgoods.storage);
                        break;
                    default:
                        printf("无效选择！\n");
                        continue;
                    }
                    // 移动到该商品在文件中的位置并写入修改后的数据
                    fseek(fp, filePos, SEEK_SET);
                    fwrite(&tempgoods, sizeof(struct item), 1, fp);
                    printf("商品信息修改成功！\n");
                    checkSingleStockWarning(&tempgoods);
                    printf("是否继续修改该商品的其他信息？（Y(是)/N（否））\n");
                    scanf("%c", &continueModifyOther);
                    scanf("%c", &continueModifyCurrent);
                    
                } while (continueModifyCurrent == 'y' || continueModifyCurrent == 'Y');
                break;
            }
        }

        if (!found) {
            printf("未找到编号为%s的商品！\n", targetId);
        }

        fclose(fp);
        printf("\n是否继续修改其他商品的信息？(Y(是)/N（否）): \n");
        scanf("%c", &continueModifyOther); 
        scanf("%c", &continueModifyOther);
    } while (continueModifyOther == 'Y' || continueModifyOther == 'y');
    printf("已退出商品修改功能！\n");
}