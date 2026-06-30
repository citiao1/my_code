#ifndef __ESTABLISH_H_
#define __ESTABLISH_H_
#define NUM 9999 
extern struct item
{
	char brand[20];//商品名字
	char id[10];//编号
	float in_price;
	float out_price;
	int storage;//商品数量
};
extern struct item goods[];
int isIdExists(const char* id);
void establish();
#endif
