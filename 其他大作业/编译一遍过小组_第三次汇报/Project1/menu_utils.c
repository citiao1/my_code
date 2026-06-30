#include "menu_utils.h"
#include <stdlib.h>  
#include <string.h>
#include <Windows.h>
#pragma comment(lib, "winmm.lib")
// 初始化菜单
Menu* initMenu(char* options[], int count) {
    Menu* menu = (Menu*)malloc(sizeof(Menu));
    menu->options = options;
    menu->count = count;

    // 计算最大选项宽度作为菜单宽度
    menu->width = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(options[i]);
        if (len > menu->width) {
            menu->width = len;
        }
    }
    menu->width += 6;  // 预留边距
    return menu;
}

// 绘制菜单边框
void drawBorder(Menu* menu, int startY) {

    printf("┌");
    for (int i = 0; i < menu->width; i++) printf("─");
    printf("┐\n");
    for (int i = 0; i < startY + menu->count + 1; i++) printf("\n");
    printf("└");
    for (int i = 0; i < menu->width; i++) printf("─");
    printf("┘\n");
}


int showMenu(Menu* menu) {
    int selected = 0;
    int key;
    int startY = 3;  // 菜单开始Y坐标

    while (1) {
        system("cls");
        SetConsoleOutputCP(936);

        // 绘制标题(居中)
        int consoleWidth = getConsoleWidth();
        char title[] = "请选择操作";
        setCursorPosition((consoleWidth - strlen(title)) / 2, 1);
        printf("%s", title);

        // 为每个选项绘制边框
        for (int i = 0; i < menu->count; i++) {
            // 每个选项占3行(上边框+内容+下边框)，间隔1行
            drawOptionBox(menu->options[i], startY + i * 4, i == selected);
        }

        // 处理按键
        key = _getch();
        if (key == 224) {  // 方向键
            key = _getch();
            if (key == 72) {  // 上箭头
                selected = (selected - 1 + menu->count) % menu->count;
                playMP3("dsgnsynth_source-synthetic-modular-synth-magic-scifi-element-analog-fx-organic-093_esm_mmogm-386261.mp3");  
            }
            else if (key == 80) {  // 下箭头
                selected = (selected + 1) % menu->count;
                playMP3("dsgnsynth_source-synthetic-modular-synth-magic-scifi-element-analog-fx-organic-093_esm_mmogm-386261.mp3");  
            }
        }
        else if (key == 13) {  // Enter键
            playMP3("clear-combo-4-394493.mp3");  
            break;
        }
    }
    return selected + 1;
}

void freeMenu(Menu* menu) {
    free(menu);
}

int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

// 移动光标到指定位置
void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 绘制单个选项框
void drawOptionBox(const char* text, int y, int isSelected) {
    int consoleWidth = getConsoleWidth();
    int boxWidth = strlen(text) + 6;  // 文本长度+左右边距
    int startX = (consoleWidth - boxWidth) / 2;  // 计算居中X坐标

    // 绘制上边框
    setCursorPosition(startX, y);
    printf("┌");
    for (int i = 0; i < boxWidth - 2; i++) printf("─");
    printf("┐");

    // 绘制内容行
    setCursorPosition(startX, y + 1);
    printf("│  %s  │", text);

    // 绘制下边框
    setCursorPosition(startX, y + 2);
    printf("└");
    for (int i = 0; i < boxWidth - 2; i++) printf("─");
    printf("┘");

    // 选中状态标记
    if (isSelected) {
        setCursorPosition(startX - 3, y + 1);
        printf("→");
    }
}
void playMP3(const char* filePath) {
    // 停止当前可能正在播放的音频
    mciSendStringA("close mp3", NULL, 0, NULL);

    // 打开并播放指定MP3文件
    char command[256];
    sprintf(command, "open \"%s\" type mpegvideo alias mp3", filePath);
    mciSendStringA(command, NULL, 0, NULL);
    mciSendStringA("play mp3", NULL, 0, NULL);
}