#include<stdio.h>
#include "establish.h"
#include "dis_all.h"
#include "shop_cart.h"
#include "user.h"
#include "user_menu.h" 
#include<windows.h>
#include<stdlib.h>
#include"modifyGoodsByID.h"
#include"deleteGoodsByID.h"

struct item goods[NUM];


int main() {
    SetConsoleOutputCP(936);
    SetConsoleCP(936);
    int modeChoice = 0;
    UserType userType;
    int key;

    
    while (1) {
        system("cls");
        int consoleWidth = getConsoleWidth();
        int startY = 5;
        char title[] = "请选择用户模式";
        setCursorPosition((consoleWidth - strlen(title)) / 2, 2);
        printf("%s", title);
        drawOptionBox("管理员模式", startY, modeChoice == 0);
        drawOptionBox("客户模式", startY + 4, modeChoice == 1);

        
        key = _getch();
        if (key == 224) {  // 方向键
            key = _getch();
            if (key == 72) {  // 上箭头
                modeChoice = (modeChoice - 1 + 2) % 2;
                playMP3("dsgnsynth_source-synthetic-modular-synth-magic-scifi-element-analog-fx-organic-093_esm_mmogm-386261.mp3");
            }   
            else if (key == 80) {  // 下箭头
                modeChoice = (modeChoice + 1) % 2;
                playMP3("dsgnsynth_source-synthetic-modular-synth-magic-scifi-element-analog-fx-organic-093_esm_mmogm-386261.mp3");
            }
        }
        else if (key == 13) {
            playMP3("clear-combo-4-394493.mp3");// Enter确认
            break;
        }
    }
    userType = (modeChoice == 0) ? ADMIN : CUSTOMER;

    // 登录/注册界面
    int loginSuccess = 0;
    int loginChoice = 0;
    while (!loginSuccess) {
        system("cls");
        int consoleWidth = getConsoleWidth();
        int startY = 5;

        // 标题居中
        char title[] = "请选择操作";
        setCursorPosition((consoleWidth - strlen(title)) / 2, 2);
        printf("%s", title);

        // 绘制选项框
        drawOptionBox("登录", startY, loginChoice == 0);
        drawOptionBox("注册", startY + 4, loginChoice == 1);

        // 处理输入
        key = _getch();
        if (key == 224) {  // 方向键
            key = _getch();
            if (key == 72) {  // 上箭头
                loginChoice = (loginChoice - 1 + 2) % 2;
                playMP3("dsgnsynth_source-synthetic-modular-synth-magic-scifi-element-analog-fx-organic-093_esm_mmogm-386261.mp3");
            }
            else if (key == 80) {  // 下箭头
                loginChoice = (loginChoice + 1) % 2;
                playMP3("dsgnsynth_source-synthetic-modular-synth-magic-scifi-element-analog-fx-organic-093_esm_mmogm-386261.mp3");
            }
        }
        else if (key == 13) {  // Enter确认
            playMP3("clear-combo-4-394493.mp3");
            if (loginChoice == 0) {
                system("cls");
                loginSuccess = login(userType);
                
            }
            else {
                system("cls");
                loginSuccess = registerUser(userType);
            }
            if (!loginSuccess) {
                printf("操作失败，按任意键继续...");
                _getch();
            }
        }
    }

    showMenuByUserType(userType);
    return 0;
}