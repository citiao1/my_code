#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "user.h"
#include "menu_utils.h"
char currentUser[20] = "";  // 存储当前登录用户名
UserType currentUserType;   // 存储当前登录用户角色
int registerAsMember() {

    FILE* fp = fopen("customers.dat", "rb+");
    if (fp == NULL) {
        perror("文件操作失败");
        return 0;
    }

    User tempUser;
    int found = 0;
    while (fread(&tempUser, sizeof(User), 1, fp) == 1) {
        if (strcmp(tempUser.username, currentUser) == 0) {
            found = 1;
            if (tempUser.isMember) {
                printf("您已经是会员了\n");
                fclose(fp);
                return 1;
            }

            
            tempUser.isMember = 1;
            
            fseek(fp, -sizeof(User), SEEK_CUR);
            fwrite(&tempUser, sizeof(User), 1, fp);
            printf("会员开通成功！结算时将自动享受9折优惠\n");
            break;
        }
    }

    fclose(fp);
    if (!found) {
        printf("用户信息不存在\n");
        return 0;
    }
    return 1;
}
// 简易哈希函数实现 - 将字符串转换为32位十六进制哈希值
void simple_hash(const char* input, char* output) {
    unsigned int hash = 0x811C9DC5; // FNV-1a初始值
    const unsigned int prime = 0x01000193; // FNV质数
    while (*input) {
        hash ^= (unsigned char)*input; // 异或当前字节
        hash *= prime; 
        input++;
    }
    // 生成8位十六进制字符串（4字节哈希值→8个十六进制字符）
    
    sprintf(output, "%08X", hash);
    output[8] = '\0';
}
const char* getUserFileName(UserType type) {
    return type == ADMIN ? "admins.dat" : "customers.dat";
}



// 清除标准输入缓冲区
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
int verifyAdminPassword(const char* inputPassword) {
    char inputHash[33];
    simple_hash(inputPassword, inputHash); // 明文→哈希

    FILE* fp = fopen("admins.dat", "rb");
    if (!fp) {
        perror("打开管理员文件失败");
        return 0;
    }

    User admin;
    int verified = 0;
    while (fread(&admin, sizeof(User), 1, fp) == 1) {
        if (strcmp(admin.username, currentUser) == 0 &&
            strcmp(admin.password_hash, inputHash) == 0) {
            verified = 1;
            break;
        }
    }
    fclose(fp);
    return verified;
}

void showAllUsers(UserType type) {
    const char* filename = getUserFileName(type);
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("\n无%s记录（文件不存在）\n", type == ADMIN ? "管理员" : "顾客");
        return;
    }

    User user;
    int hasRecord = 0;
    rewind(fp);  // 确保从开头读取
    printf("\n=====%s列表=====\n", type == ADMIN ? "管理员" : "顾客");
    while (fread(&user, sizeof(User), 1, fp) == 1) {
        hasRecord = 1;
        printf("用户名：%s\n", user.username);
        if (type == ADMIN && strcmp(user.username, currentUser) == 0) {
            printf("（当前登录管理员）\n");
        }
    }

    if (!hasRecord) {
        printf("无%s记录\n", type == ADMIN ? "管理员" : "顾客");
    }
    fclose(fp);
}
// 管理员修改密码
int adminChangePassword(const char* targetUser, UserType targetType,
    const char* adminPassword, const char* newPassword) {
    // 管理员密码验证
    if (!verifyAdminPassword(adminPassword)) {
        return -1;
    }

    const char* filename = getUserFileName(targetType);
    FILE* fp = fopen(filename, "rb+");
    if (!fp) {
        printf("错误：%s文件不存在，无法修改密码\n", filename);
        return 0;
    }

    User user;
    int found = 0;
    long userPos = -1;

    // 确保从文件开头开始遍历
    rewind(fp);  
    while (fread(&user, sizeof(User), 1, fp) == 1) {
        if (strcmp(user.username, targetUser) == 0) {
            userPos = ftell(fp) - sizeof(User);
            found = 1;
            break;
        }
    }

    if (found && userPos != -1) {
        // 计算新密码哈希并写入
        simple_hash(newPassword, user.password_hash);
        if (fseek(fp, userPos, SEEK_SET) != 0) {  
            perror("定位用户记录失败");
            fclose(fp);
            return 0;
        }
        if (fwrite(&user, sizeof(User), 1, fp) != 1) {  
            perror("写入新密码失败");
            fclose(fp);
            return 0;
        }
        fclose(fp);
        return 1;  
    }
    else {
        fclose(fp);
        return 0;  
    }
}
int adminDeleteUser(const char* targetUser, UserType targetType, const char* adminPassword) {
    // 验证管理员密码
    if (!verifyAdminPassword(adminPassword)) {
        return -1;
    }

    // 不能删除当前登录的管理员自己
    if (targetType == ADMIN && strcmp(targetUser, currentUser) == 0) {
        return -2; 
    }

    const char* filename = getUserFileName(targetType);
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("打开%s文件失败，无法删除用户\n", filename);
        return 0;
    }

    // 创建临时文件存储保留的用户
    FILE* tempFp = fopen("temp_users.dat", "wb");
    if (!tempFp) {
        printf("创建临时文件失败，无法删除用户\n");
        fclose(fp);
        return 0;
    }

    User user;
    int found = 0;

    while (fread(&user, sizeof(User), 1, fp) == 1) {
        if (strcmp(user.username, targetUser) == 0) {
            found = 1; // 标记找到要删除的用户，不写入临时文件
        }
        else {
            fwrite(&user, sizeof(User), 1, tempFp); // 其他用户写入临时文件
        }
    }

    fclose(fp);
    fclose(tempFp);

    // 如果找到要删除的用户，则替换原文件
    if (found) {
        remove(filename);
        rename("temp_users.dat", filename);
        return 1;
    }
    else {
        remove("temp_users.dat"); // 清理临时文件
        return 0; 
    }
}
// 顾客修改自己的密码（新增）
int customerChangeOwnPassword(const char* oldPassword, const char* newPassword) {
    char oldHash[33];
    simple_hash(oldPassword, oldHash); // 旧明文→哈希

    FILE* fp = fopen("customers.dat", "rb+");
    if (!fp) return 0;

    User user;
    int success = 0;
    long userPos = -1;

    while (fread(&user, sizeof(User), 1, fp) == 1) {
        if (strcmp(user.username, currentUser) == 0) {
            userPos = ftell(fp) - sizeof(User); // 记录用户位置
            
            if (strcmp(user.password_hash, oldHash) == 0) {
                // 新密码→哈希值存储
                simple_hash(newPassword, user.password_hash);
                fseek(fp, userPos, SEEK_SET); // 定位到用户记录
                fwrite(&user, sizeof(User), 1, fp);
                success = 1;
            }
            break;
        }
    }
    fclose(fp);
    return success;
}


int registerUser(UserType type) {
    User user;
    FILE* fp;
    const char* filename = getUserFileName(type);
    char password[20];

    printf("请输入用户名（不超过19字符）: ");
    if (scanf("%19s", user.username) != 1) {
        printf("输入错误！\n");
        clearInputBuffer();
        return 0;
    }

    // 密码复杂度检查
    int validPassword = 0;
    while (!validPassword) {
        printf("请输入密码（6-19字符，需包含字母和数字）: ");
        if (scanf("%19s", password) != 1) {
            printf("输入错误！\n");
            clearInputBuffer();
            return 0;
        }

        int len = strlen(password);
        int hasLetter = 0, hasNumber = 0;

        if (len < 6) {
            printf("密码长度不能少于6个字符！\n");
            continue;
        }

        for (int i = 0; i < len; i++) {
            if (isalpha(password[i])) hasLetter = 1;
            if (isdigit(password[i])) hasNumber = 1;
        }

        if (!hasLetter || !hasNumber) {
            printf("密码必须同时包含字母和数字！\n");
        }
        else {
            validPassword = 1;
        }
    }

    // 对密码进行哈希处理
    simple_hash(password, user.password_hash);
    user.type = type;
    user.isMember = 0;
    // 检查用户名重复
    fp = fopen(filename, "rb");
    if (fp != NULL) {
        User temp;
        while (fread(&temp, sizeof(User), 1, fp) == 1) {
            if (strcmp(temp.username, user.username) == 0) {
                printf("用户名已存在!\n");
                fclose(fp);
                return 0;
            }
        }
        fclose(fp);
    }

    // 写入新用户
    fp = fopen(filename, "ab");
    if (fp == NULL) {
        perror("注册失败：文件打开失败");
        return 0;
    }

    if (fwrite(&user, sizeof(User), 1, fp) != 1) {  // 校验写入是否成功
        printf("注册失败：数据写入错误\n");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    // 记录当前登录状态
    strcpy(currentUser, user.username);
    currentUserType = type;
    printf("注册成功！当前登录用户：%s\n", currentUser);
    return 1;
}

// 登录功能
int login(UserType type) {
    User user;
    FILE* fp;
    char username[20], password[20], password_hash[33];
    const char* filename = getUserFileName(type);
    char retry;

    do {
        printf("请输入用户名: ");
        if (scanf("%19s", username) != 1) {
            printf("输入错误！\n");
            clearInputBuffer();
            return 0;
        }

        printf("请输入密码: ");
        if (scanf("%19s", password) != 1) {
            printf("输入错误！\n");
            clearInputBuffer();
            return 0;
        }

        // 对输入的密码进行哈希处理
        simple_hash(password, password_hash);

        fp = fopen(filename, "rb");
        if (fp == NULL) {
            perror("登录失败：无此类型用户文件");
            return 0;
        }

        int found = 0;
        while (fread(&user, sizeof(User), 1, fp) == 1) {
            if (strcmp(user.username, username) == 0 &&
                strcmp(user.password_hash, password_hash) == 0) {
                found = 1;
                break;
            }
        }

        fclose(fp);

        if (found) {
            // 记录当前登录状态
            strcpy(currentUser, user.username);
            currentUserType = type;
            printf("登录成功！当前登录用户：%s（%s）\n",
                currentUser, type == ADMIN ? "管理员" : "顾客");
            return 1;
        }
        else {
            printf("用户名或密码错误!\n");
            printf("是否重新登录（Y/N）: ");
            clearInputBuffer();  
            retry = getchar();
            if (toupper(retry) != 'Y') {
                return 0;  
            }
            clearInputBuffer();
        }
    } while (1);
}

