#include<stdio.h>
#include"shop_cart.h"
#include"establish.h"
#include"profit_analysis.h"  
#include<windows.h>
#include<stdlib.h>
#include<string.h>
#include "dis_all.h"
#include "user.h"
#include "menu_utils.h"
struct item goods[NUM];
struct item_node* cart = NULL;

void display() {

	struct  item_node* p = cart;
	if (p == NULL) {
		printf("购物车为空\n");
		return;
	}
	while (p != NULL) {
		printf("——————————————————————\n");
		printf("编号	品名	售价	数量\n");
		printf("%s%9s%9.2f%9d\n", p->wanted.id, p->wanted.brand, p->wanted.out_price, p->amount);
		p = p->next;

	}

}

void add() {
	FILE* fp;
	int n;
	int i;
	char str[20];
	char choice = ' ';
	char choice2 = ' ';
	struct item_node* p1;
	struct item_node* p;
	struct item tempGoods;
	dis_all(0);
	do {
		printf("输入所需物品的名称或者编号：\n");
		fflush(stdin);
		scanf_s("%s", str, 20);
		if ((fp = fopen("goods", "r")) == NULL) {
			printf("文件打开失败\n");
			continue;
		}
		for (i = 0; fread(goods + i, sizeof(struct item), 1, fp) != 0; i++) {
			if ((strcmp(goods[i].brand, str) == 0) || (strcmp(goods[i].id, str) == 0) && goods[i].storage != 0) {
				printf("已经找到所需物品：\n");
				printf("——————————————————————\n");
				printf("编号	品名	售价	库存数量\n");
				printf("%s%9s%9.2f%9d\n", goods[i].id, goods[i].brand, goods[i].out_price, goods[i].storage);
				printf("请输入所需购买的数量：");
				scanf("%d", &n);
				if (n > goods[i].storage) {
					printf("库存不足\n");
					break;
				}
				printf("是否购买？（Y/N）:");
				scanf("%c", &choice);
				scanf("%c", &choice);
				if (choice == 'Y' || choice == 'y') {
					p1 = (struct item_node*)malloc(sizeof(struct item_node));//开辟内存
					if (p1 == NULL) {
						printf("内存分配失败\n");
						exit(1);
					}
					p1->amount = n;//存入需要的商品数量
					p1->wanted = goods[i];//存入商品信息
					p1->next = NULL;
					p = cart;
					if (cart == NULL) {
						cart = p1;
					}
					else {
						while (p->next != NULL)
							p = p->next;
						p1->next = p->next;
						p->next = p1;

					}
				}
				break;
			}


		}
		if (i == NUM) {
			printf("未找到所需商品\n");

		}
		fclose(fp);
		printf("是否继续购物？（Y/N）:\n");
		fflush(stdin);
		scanf("%c", &choice2);
		scanf("%c", &choice2);

	} while (choice2 == 'Y' || choice2 == 'y');
}

void shop_cart() {
	while (1) {
		
		char* options[] = { "查看所有商品信息", "显示当前购物车列表", "添加商品", "删除购物车商品", "返回" };
		Menu* cartMenu = initMenu(options, 5);
		int select = showMenu(cartMenu);
		freeMenu(cartMenu);

		switch (select) {
		case 1:
			dis_all(0);
			system("pause");  
			break;
		case 2:
			system("cls");
			display();
			system("pause");
			break;
		case 3:
			system("cls");
			add();
			system("pause");
			break;
		case 4:
			system("cls");
			deleteItem();
			system("pause");
			break;
		case 5:
			system("cls");
			return;
		}
	}
}





void calculate() {
	float total = 0, pay = 0;
	struct item_node* cartTemp = cart;  // 用临时指针遍历，保留原购物车数据
	FILE* fp;
	printf("以下是您的订单\n");
	display();

	// 读取商品数据
	if ((fp = fopen("goods", "r")) == NULL) {
		printf("文件打开失败\n");
		return;
	}
	int goodsCount = 0;
	for (goodsCount = 0; fread(goods + goodsCount, sizeof(struct item), 1, fp) != 0; goodsCount++);
	fclose(fp);

	// 计算总金额并更新库存
	while (cartTemp != NULL) {
		total += cartTemp->wanted.out_price * cartTemp->amount;
		// 更新库存
		for (int i = 0; i < goodsCount; i++) {
			if (strcmp(goods[i].id, cartTemp->wanted.id) == 0) {
				goods[i].storage -= cartTemp->amount;
				break;
			}
		}
		cartTemp = cartTemp->next;
	}
	int isMember = 0;
	if (currentUserType == CUSTOMER) {
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
	}
	float discount = isMember ? 0.9f : 1.0f;
	float finalTotal = total * discount;
	if (isMember) {
		printf("会员优惠：9折\n");
		printf("原价：%.2f 元，折后价：%.2f 元\n", total, finalTotal);
	}
	else {
		finalTotal = total;
		printf("总价：%.2f 元\n", finalTotal);
	}
	char showQrCode;
	int useQrPayment = 0;
	printf("\n是否需要展示收款码？(Y/N)：");
	scanf(" %c", &showQrCode);
	if (showQrCode == 'Y' || showQrCode == 'y') {
		useQrPayment = 1;
		showQRCode(finalTotal);
		pay = finalTotal;
		playMP3("cashier-quotka-chingquot-sound-effect-129698.mp3");
		printf("\n已确认收款码支付完成，金额：%.2f 元\n", pay);
	}
	// 处理支付
	if (!useQrPayment) {
	printf("总计: %7.2f\n", finalTotal);
	printf("\n请输入实付金额:");
	scanf("%f", &pay);
	playMP3("cashier-quotka-chingquot-sound-effect-129698.mp3");
	printf("实际支付:%7.2f\t\t找零:%7.2f\n", pay, pay - finalTotal);
	}
	

	// 保存更新后的库存
	if ((fp = fopen("goods", "w")) == NULL) {
		printf("文件写入失败\n");
		return;
	}
	fwrite(goods, sizeof(struct item), goodsCount, fp);
	fclose(fp);

	// 生成订单
	Order* newOrder = (Order*)malloc(sizeof(Order));
	generateOrderId(newOrder->orderId);
	strcpy(newOrder->username, currentUser);
	newOrder->orderTime = time(NULL);
	newOrder->totalProfit = 0;
	newOrder->items = NULL;
	OrderItem* lastItem = NULL;

	struct item_node* p = cart;
	while (p != NULL) {
		// 会员价利润计算
		float itemPrice = isMember ? (p->wanted.out_price * 0.9f) : p->wanted.out_price;
		float itemProfit = (itemPrice - p->wanted.in_price) * p->amount;
		newOrder->totalProfit += itemProfit;

		// 订单商品信息录入
		OrderItem* item = (OrderItem*)malloc(sizeof(OrderItem));
		strcpy(item->productId, p->wanted.id);
		strcpy(item->productBrand, p->wanted.brand);
		item->quantity = p->amount;
		item->inPrice = p->wanted.in_price;
		item->outPrice = itemPrice;  // 记录实际售价（含折扣）
		item->profit = itemProfit;
		item->next = NULL;

		if (newOrder->items == NULL) {
			newOrder->items = item;
		}
		else {
			lastItem->next = item;
		}
		lastItem = item;
		p = p->next;
	}


	saveOrder(newOrder);

	// 释放内存
	OrderItem* tempItem;
	while (newOrder->items != NULL) {
		tempItem = newOrder->items;
		newOrder->items = newOrder->items->next;
		free(tempItem);
	}
	free(newOrder);

	cart = NULL;
}


void deleteItem() {
	if (cart == NULL) {
		printf("购物车为空，无法删除商品！\n");
		system("pause");
		system("cls");
		return;
	}

	char target[20];
	printf("请输入要删除的商品ID或商品名称：");
	fflush(stdin);
	scanf("%s", target);

	struct item_node* p = cart;       // 当前节点
	struct item_node* prev = NULL;    // 前驱节点
	int found = 0;

	// 遍历购物车查找匹配商品
	while (p != NULL) {
		// 匹配商品ID或名称
		if (strcmp(p->wanted.id, target) == 0 || strcmp(p->wanted.brand, target) == 0) {
			found = 1;
			
			if (prev == NULL) {
				
				cart = p->next;
			}
			else {
				
				prev->next = p->next;
			}
			// 释放内存
			free(p);
			printf("商品删除成功！\n");
			break; 
		}
		
		prev = p;
		p = p->next;
	}

	if (!found) {
		printf("未找到该商品，删除失败！\n");
	}

	system("pause");
	system("cls");
}
void showQRCode(float amount) {
	ShellExecuteA(NULL, "open", "payment_qrcode.png", NULL, NULL, SW_SHOWNORMAL);
	printf("\n请扫描二维码支付 %.2f 元\n", amount);
	printf("支付完成后按任意键继续...\n");
	_getch();
}