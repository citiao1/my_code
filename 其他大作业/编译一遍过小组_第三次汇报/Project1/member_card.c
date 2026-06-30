
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "user.h"
#include "shop_cart.h"
#include "profit_analysis.h"  
#include "menu_utils.h"
extern char currentUser[20];
extern struct item goods[];
extern struct item_node* cart;

//查询会员余额
float getMemberBalance() {
    FILE* fp = fopen("customers.dat", "rb");
    if (fp == NULL) {
        printf("会员数据文件打开失败！\n");
        return -1.0f;
    }

    User tempUser;
    float balance = 0.0f;
    while (fread(&tempUser, sizeof(User), 1, fp) == 1) {
        if (strcmp(tempUser.username, currentUser) == 0) {
            balance = tempUser.balance;
            
            break;
        }
    }
    fclose(fp);
    return balance;
}


void rechargeMemberBalance() {
    float rechargeAmount;
    printf("请输入充值金额（元）：");
    while (1) {
        if (scanf("%f", &rechargeAmount) != 1) {
            printf("输入格式错误，请输入数字：");
            while (getchar() != '\n'); 
        }
        else if (rechargeAmount <= 0) {
            printf("充值金额不能为负数，请重新输入：");
        }
        else {
            break;
        }
    }

    FILE* fp = fopen("customers.dat", "rb+");
    if (fp == NULL) {
        perror("会员数据文件打开失败");
        printf("充值失败\n");
        return;
    }

    User tempUser;
    int found = 0;
    while (fread(&tempUser, sizeof(User), 1, fp) == 1) {
        if (strcmp(tempUser.username, currentUser) == 0) {
            found = 1;
            
            if (tempUser.balance < -1000000.0f) {
                printf("检测到异常余额，已自动重置为0.00元\n");
                tempUser.balance = 0.0f;
            }
            tempUser.balance += rechargeAmount;
            
            
            if (fseek(fp, -sizeof(User), SEEK_CUR) != 0) {
                perror("充值失败：无法定位用户位置");
                fclose(fp);
                return;
            }
            size_t writeResult = fwrite(&tempUser, sizeof(User), 1, fp);
            
            if (writeResult != 1) {
                perror("fwrite失败（无法写入用户数据）");
                fclose(fp);
                return;
            }
            printf("充值成功！当前会员余额：%.2f 元\n", tempUser.balance);
            break;
        }
    }

    if (!found) {
        printf("未找到当前用户（%s），充值失败\n", currentUser);
    }
    fclose(fp);
}

//会员卡结算
int settleWithMemberCard() {
    // 计算订单折后总价
    float total = 0.0f;
    struct item_node* cartTemp = cart;
    if (cartTemp == NULL) {
        printf("购物车为空，无法结算！\n");
        return 0;
    }

    int isMember = 0;
    FILE* userFp = fopen("customers.dat", "rb");
    if (userFp != NULL) {
        User tempUser;
        while (fread(&tempUser, sizeof(User), 1, userFp) == 1) {
            if (strcmp(tempUser.username, currentUser) == 0) {
                isMember = tempUser.isMember;
                break;
            }
        }
        fclose(userFp);
    }
    float discount = isMember ? 0.9f : 1.0f;
    while (cartTemp != NULL) {
        total += cartTemp->wanted.out_price * cartTemp->amount * discount;
        cartTemp = cartTemp->next;
    }

    // 校验余额是否足够
    float balance = getMemberBalance();
    if (balance < 0) return 0;
    
    printf("订单折后总价：%.2f 元\n当前会员余额：%.2f 元\n", total, balance);
    if (balance < total) {
        printf("会员余额不足，结算失败！\n");
        return 0;
    }
    
    // 扣减余额
    FILE* fp = fopen("customers.dat", "rb+");
    if (fp == NULL) {
        printf("会员数据文件打开失败，结算失败！\n");
        return 0;
    }
    User tempUser;
    while (fread(&tempUser, sizeof(User), 1, fp) == 1) {
        if (strcmp(tempUser.username, currentUser) == 0) {
            tempUser.balance -= total;
            fseek(fp, -sizeof(User), SEEK_CUR);
            fwrite(&tempUser, sizeof(User), 1, fp);
            break;
        }
    }
    fclose(fp);

    // 更新库存
    FILE* goodsFp = fopen("goods", "r");
    int goodsCount = 0;
    for (goodsCount = 0; fread(goods + goodsCount, sizeof(struct item), 1, goodsFp) != 0; goodsCount++);
    fclose(goodsFp);

    cartTemp = cart;
    while (cartTemp != NULL) {
        for (int i = 0; i < goodsCount; i++) {
            if (strcmp(goods[i].id, cartTemp->wanted.id) == 0) {
                goods[i].storage -= cartTemp->amount;
                break;
            }
        }
        cartTemp = cartTemp->next;
    }

    goodsFp = fopen("goods", "w");
    fwrite(goods, sizeof(struct item), goodsCount, goodsFp);
    fclose(goodsFp);

    // 生成订单
    Order* newOrder = (Order*)malloc(sizeof(Order));
    generateOrderId(newOrder->orderId);
    strcpy(newOrder->username, currentUser);
    newOrder->orderTime = time(NULL);
    newOrder->totalProfit = 0;
    newOrder->items = NULL;
    OrderItem* lastItem = NULL;

    cartTemp = cart;
    while (cartTemp != NULL) {
        float itemPrice = cartTemp->wanted.out_price * discount;
        float itemProfit = (itemPrice - cartTemp->wanted.in_price) * cartTemp->amount;
        newOrder->totalProfit += itemProfit;

        OrderItem* item = (OrderItem*)malloc(sizeof(OrderItem));
        strcpy(item->productId, cartTemp->wanted.id);
        strcpy(item->productBrand, cartTemp->wanted.brand);
        item->quantity = cartTemp->amount;
        item->inPrice = cartTemp->wanted.in_price;
        item->outPrice = itemPrice;
        item->profit = itemProfit;
        item->next = NULL;

        if (newOrder->items == NULL) newOrder->items = item;
        else lastItem->next = item;
        lastItem = item;
        cartTemp = cartTemp->next;
    }

    saveOrder(newOrder);
    OrderItem* tempItem;
    while (newOrder->items != NULL) {
        tempItem = newOrder->items;
        newOrder->items = newOrder->items->next;
        free(tempItem);
    }
    free(newOrder);

    // 清空购物车
    playMP3("cashier-quotka-chingquot-sound-effect-129698.mp3");
    printf("会员卡结算成功！扣减余额：%.2f 元，剩余余额：%.2f 元\n", total, getMemberBalance());
    cart = NULL;
    return 1;
}


void memberCenter() {
    int memberChoice;
    int isMember = 0;

    // 检查当前用户是否为会员
    FILE* userFp = fopen("customers.dat", "rb");
    if (userFp != NULL) {
        User tempUser;
        while (fread(&tempUser, sizeof(User), 1, userFp) == 1) {
            if (strcmp(tempUser.username, currentUser) == 0) {
                isMember = tempUser.isMember;
                break;
            }
        }
        fclose(userFp);
    }

    // 非会员处理
    if (!isMember) {
        char confirm;
        printf("当前非会员，是否开通会员？(Y/N)：");
        scanf(" %c", &confirm);
        if (confirm == 'Y' || confirm == 'y') {
            registerAsMember();
           
        }
    }

    // 会员菜单
    do {
        system("cls");
        char* memberOptions[] = {
            "会员充值",
            "查询会员余额",
            "会员结算购物车",
            "返回上一级"
        };
        Menu* memberMenu = initMenu(memberOptions, 4);
        memberChoice = showMenu(memberMenu);
        freeMenu(memberMenu);

        // 菜单逻辑处理
        switch (memberChoice) {
        case 1:
            system("cls");
            rechargeMemberBalance();
            system("pause");
            break;
        case 2:
            system("cls");
            printf("当前会员余额：%.2f 元\n", getMemberBalance());
            system("pause");
            break;
        case 3:
            system("cls");
            settleWithMemberCard();
            system("pause");
            break;
        case 4:
            system("cls");
            return;  
        }
    } while (1);
}