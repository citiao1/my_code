#ifndef __USER_H_
#define __USER_H_


// 用户类型枚举
typedef enum {
    ADMIN,
    CUSTOMER
} UserType;
extern char currentUser[20];
extern UserType currentUserType;
// 用户结构体
typedef struct {
    char username[20];       // 用户名
    char password_hash[33];  // 密码哈希
    UserType type;           // 用户角色
    int isMember;            // 是否为会员（0=非会员，1=会员）
    float balance;           // 会员余额
} User;
extern char currentUser[20];
// 函数声明
int login(UserType type);       // 登录功能
int registerUser(UserType type);// 注册功能
void userMenu();                // 用户模式选择菜单
int registerAsMember();
int adminDeleteUser(const char* targetUser, UserType targetType, const char* adminPassword);
// 新增密码修改相关函数声明
int verifyAdminPassword(const char* password);  // 管理员密码验证
int adminChangePassword(const char* targetUser, UserType targetType, const char* adminPassword, const char* newPassword);
int customerChangeOwnPassword(const char* oldPassword, const char* newPassword);
void showAllUsers(UserType type); // 显示所有用户信息

#endif