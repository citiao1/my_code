#ifndef __SHOP_CART_
#define __SHOP_CART_
#include "establish.h"

extern struct item_node
{
	struct item wanted;
	int amount;
	struct item_node* next;
};

extern struct item_node* cart;

void display();
void add();
void shop_cart();
int  cart_menu();
void calculate();
void deleteItem();
void showQRCode(float amount);

#endif