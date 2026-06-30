#ifndef __MENU_UTILS_H_
#define __MENU_UTILS_H_

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <windows.h>

// 菜单结构体
typedef struct {
    char** options;  // 选项文本数组
    int count;       // 选项数量
    int width;       // 菜单宽度
} Menu;


Menu* initMenu(char* options[], int count);
void drawBorder(Menu* menu, int startY);
int showMenu(Menu* menu);
void freeMenu(Menu* menu);
int getConsoleWidth();

void setCursorPosition(int x, int y);
void drawOptionBox(const char* text, int y, int isSelected);
void playMP3(const char* filePath);
#endif