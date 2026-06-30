#include<stdio.h>
#include "user_menu.h"
#include "establish.h"
#include "dis_all.h"
#include "shop_cart.h"
#include<stdlib.h>
#include "profit_analysis.h"
#include "order_view.h"
#include "member_card.h"
#include "user_consumption.h"
#include "menu_utils.h"
#include "modifyGoodsByID.h"
#include "deleteGoodsByID.h"
extern struct item goods[];
extern struct item_node* cart;

void adminDeleteUserMenu() {
    char adminPwd[20], targetUser[20];
    int targetType;

    printf("请输入当前管理员密码(用于验证):");
    scanf("%19s", adminPwd);

    // 显示所有可删除的用户
    printf("\n===== 可删除的管理员账号(除当前登录) =====");
    showAllUsers(ADMIN);
    printf("\n===== 可删除的顾客账号 =====");
    showAllUsers(CUSTOMER);

    printf("\n请输入要删除的用户名:");
    scanf("%19s", targetUser);
    printf("请选择用户类型(1=管理员, 2=顾客):");
    scanf("%d", &targetType);

    // 执行删除操作
    int res = adminDeleteUser(targetUser,
        (targetType == 1 ? ADMIN : CUSTOMER),
        adminPwd);

    // 结果显示
    if (res == -1) printf("管理员密码验证失败，无法删除\n");
    else if (res == -2) printf("不能删除当前登录的管理员自己\n");
    else if (res == 1) printf("用户删除成功\n");
    else printf("目标用户不存在，请检查用户名\n");
}
void adminModifyPasswordMenu() {
    char adminPwd[20], targetUser[20], newPwd[20];
    int targetType;

    printf("请输入您的管理员密码(身份验证):");
    scanf("%19s", adminPwd);

    // 显示所有可修改的账户（包括自己）
    printf("\n===== 可修改的管理员账户（包括您自己） =====");
    showAllUsers(ADMIN);
    printf("\n===== 可修改的顾客账户 =====");
    showAllUsers(CUSTOMER);

    printf("\n请输入要修改的用户名（可输入您自己的用户名）:");
    scanf("%19s", targetUser);
    printf("请选择用户类型(1=管理员, 2=顾客):");
    scanf("%d", &targetType);
    printf("请输入新密码:");
    scanf("%19s", newPwd);

    // 执行修改
    int res = adminChangePassword(targetUser,
        (targetType == 1 ? ADMIN : CUSTOMER),
        adminPwd, newPwd);

    // 结果提示
    if (res == -1) printf("管理员密码验证失败，无法修改！\n");
    else if (res == 1) printf("密码修改成功！\n");
    else printf("目标用户不存在，请检查用户名！\n");
}
void adminShowAllUsersMenu() {
    printf("\n====================================\n");
    printf("           所有用户信息列表          \n");
    printf("====================================\n");

    // 先显示管理员信息
    printf("\n【管理员账户列表】\n");
    showAllUsers(ADMIN); 

    // 再显示顾客信息
    printf("\n【顾客账户列表】\n");
    showAllUsers(CUSTOMER);  

    printf("\n====================================\n");
    printf("           显示完毕                 \n");
    printf("====================================\n");
}


void showMenuByUserType(UserType type) {
    playMP3("furievox-105000.mp3");
    int select;
    while (1) {
       
        char** options;
        int optionCount;
        if (type == ADMIN) {
            char* adminOptions[] = {
                "商品管理",       // 子菜单：包含添加/显示/修改/删除商品
                "用户管理",       // 子菜单：包含查询消费/修改密码/删除用户/显示用户
                "利润分析",       
                "退出"            // 退出功能
            };
            options = adminOptions;
            optionCount = 4;
        }
        else {
            char* customerOptions[] = {
                "购物车", "结算", "会员中心", "查看历史订单", "修改个人密码","退出"
            };
            options = customerOptions;
            optionCount = 6;
        }
        Menu* mainMenu = initMenu(options, optionCount);
        select = showMenu(mainMenu);
        freeMenu(mainMenu);

        // 处理选择
        switch (select) {
        case 1:
            if (type == ADMIN) { system("cls"); adminGoodsSubMenu();
            }
			else { shop_cart(); }
            system("pause");  // 等待用户确认
            system("cls");    // 确认后再清屏
            break;
        case 2:
            if (type == ADMIN) adminUserSubMenu();
            else {
                system("cls");
                calculate();
            }
            system("pause");
            system("cls");
            break;
        case 3:
            if (type == ADMIN) profitAnalysisMenu();
            else memberCenter();
            system("pause");
            system("cls");
            break;
        case 4:
            if (type == ADMIN) {
                playMP3("dad-says-bye-bye-113119.mp3");
                system("cls");
                printf("感谢使用，再见！\n");
                system("pause");
                exit(0);
            }
        
            else viewHistoryOrders();
            system("pause");
            system("cls");
            break;
        case 5:
            
			    system("cls");
                char oldPwd[20], newPwd[20], confirmPwd[20];

                printf("请输入当前密码进行验证: ");
                scanf("%19s", oldPwd);

                
                int valid = 0;
                while (!valid) {
                    printf("请输入新密码(6-19位，含字母和数字): ");
                    scanf("%19s", newPwd);
                    printf("请确认新密码: ");
                    scanf("%19s", confirmPwd);

                    if (strcmp(newPwd, confirmPwd) != 0) {
                        printf("两次密码不一致！\n");
                        continue;
                    }

                    int len = strlen(newPwd);
                    int hasLetter = 0, hasNumber = 0;
                    for (int i = 0; i < len; i++) {
                        if (isalpha(newPwd[i])) hasLetter = 1;
                        if (isdigit(newPwd[i])) hasNumber = 1;
                    }
                    if (len < 6 || !hasLetter || !hasNumber) {
                        printf("密码不符合要求！\n");
                    }
                    else {
                        valid = 1;
                    }
                }

                // 执行修改
                if (customerChangeOwnPassword(oldPwd, newPwd)) {
                    printf("密码修改成功！\n");
                }
                else {
                    printf("原密码错误，修改失败！\n");
                }
                break;
            
            system("pause");
            system("cls");
            break;
        case 6:
            
                playMP3("dad-says-bye-bye-113119.mp3");
                system("cls");
                printf("感谢使用，再见！\n");
                system("pause");
                exit(0);
            
            system("pause");
            system("cls");
            break;
		
        }
    }
}
void adminGoodsSubMenu() {
    while (1) {
        char* goodsOptions[] = {
            "添加商品信息",
            "显示商品信息",
            "修改商品信息",
            "删除商品信息",
            "返回上一级"
        };
        Menu* goodsMenu = initMenu(goodsOptions, 5);
        int select = showMenu(goodsMenu);
        freeMenu(goodsMenu);

        switch (select) {
        case 1:  // 添加商品
            system("cls");
            establish();
            system("pause");
            system("cls");
            break;
        case 2:  // 显示商品
            dis_all(1);
            system("pause");
            system("cls");
            break;
        case 3:  // 修改商品
            system("cls");
            modifyGoodsByID();
            system("pause");
            system("cls");
            break;
        case 4:  // 删除商品
            deleteGoodsByID();
            system("pause");
            system("cls");
            break;
        case 5:  // 返回主菜单
            system("cls");
            return;
        }
    }
}
// 管理员用户管理子菜单
void adminUserSubMenu() {
    while (1) {
        char* userOptions[] = {
            "查询用户消费记录",
            "修改用户密码",
            "删除用户",
            "显示所有用户信息",
            "返回上一级"
        };
        Menu* userMenu = initMenu(userOptions, 5);
        int select = showMenu(userMenu);
        freeMenu(userMenu);

        switch (select) {
        case 1:  // 查询用户消费记录
            system("cls");
            show_user_consumption_chart();
            system("pause");
            system("cls");
            break;
        case 2:  // 修改用户密码
            system("cls");
            adminModifyPasswordMenu();
            system("pause");
            system("cls");
            break;
        case 3:  // 删除用户
            system("cls");
            adminDeleteUserMenu();
            system("pause");
            system("cls");
            break;
        case 4:  // 显示所有用户
            system("cls");
            adminShowAllUsersMenu();
            system("pause");
            system("cls");
            break;
        case 5:  // 返回主菜单
            system("cls");
            return;
        }
    }
}