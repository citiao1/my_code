#ifndef __DR16_H
#define __DR16_H

#include "main.h"

#define Key_W 0
#define Key_S 1
#define Key_D 2
#define Key_A 3
#define Key_Shift 4
#define Key_Ctrl 5
#define Key_Q 6
#define Key_E 7
#define Key_R 8
#define Key_F 9
#define Key_G 10
#define Key_Z 11
#define Key_X 12
#define Key_C 13
#define Key_V 14
#define Key_B 15
#define PASSING 0
#define LAST 1
#define KEY_PASSING 0 
#define KEY_WITH_SHIFT 2
#define KEY_WITH_CTRL 1



#pragma pack(1)

typedef union
{
    struct // 用于访问键盘状态
    {
        uint16_t w : 1;
        uint16_t s : 1;
        uint16_t a : 1;
        uint16_t d : 1;
        uint16_t shift : 1;
        uint16_t ctrl : 1;
        uint16_t q : 1;
        uint16_t e : 1;
        uint16_t r : 1;
        uint16_t f : 1;
        uint16_t g : 1;
        uint16_t z : 1;
        uint16_t x : 1;
        uint16_t c : 1;
        uint16_t v : 1;
        uint16_t b : 1;
    };
    uint16_t keys; // 用于memcpy而不需要进行强制类型转换
} Key_t;

typedef struct
{
    struct
    {
        uint16_t ch0;                   //右水平
        uint16_t ch1;                   //右竖直
        uint16_t ch2;                   //左水平
        uint16_t ch3;                   //左竖直
        uint8_t s1;                     //左侧开关
        uint8_t s2;                     //右侧开关
    } rc;

    struct
    {
        int16_t x;                      //水平方向
        int16_t y;                      //竖直方向
        int16_t z;                      //垂直方向
        uint8_t press_l;                //按下鼠标左键
        uint8_t press_r;                //按下鼠标右键
    } mouse;

    Key_t key[3];

    uint8_t key_count[3][16];


} RC_Ctl_t;

#pragma pack(0)

RC_Ctl_t *Dbus_to_rc(uint8_t pData[]);
uint8_t *GiveSbusBuff();
RC_Ctl_t *GetRCData();

#endif // !__DR16_H
