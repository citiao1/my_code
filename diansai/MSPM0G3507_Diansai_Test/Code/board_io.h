#ifndef BOARD_IO_H
#define BOARD_IO_H

#include <stdint.h>

#define BOARD_KEY_COUNT 3U

typedef enum
{
    BOARD_KEY_K1 = 0, /* bit0，实测对应母板 K1 / PB15。 */
    BOARD_KEY_K2,     /* bit1，实测对应母板 K2 / PB14。 */
    BOARD_KEY_K3      /* bit2，PB16。 */
} BoardKeyId;

/*
 * 每个掩码的 bit0/bit1/bit2 分别对应 K1/K2/K3。
 * pressed/released 表示消抖后的边沿；short_press 在短按释放时产生；
 * long_press 在持续按住 800 ms 时产生一次，长按释放不会再产生 short_press。
 */
typedef struct
{
    uint8_t pressed_mask;
    uint8_t released_mask;
    uint8_t short_press_mask;
    uint8_t long_press_mask;
} BoardKeyEvents;

/* 初始化 PB14/PB15/PB16 上拉输入和 PA28 蜂鸣器输出。 */
void BoardIo_Init(uint32_t now_ms);

/* 每 10 ms 左右调用一次，完成 30 ms 消抖、长按判断和蜂鸣器时序推进。 */
void BoardIo_Update(uint32_t now_ms);

/* 取走目前累计的全部按键事件；读取后事件缓存清零。 */
BoardKeyEvents BoardIo_TakeKeyEvents(void);

/* 返回当前消抖后的按住状态掩码，不会清除任何事件。 */
uint8_t BoardIo_GetPressedMask(void);

/*
 * 将非阻塞提示音加入队列：响 on_ms、停 off_ms，共重复 repeat_count 次。
 * 参数无效或队列已满返回 0；真正的开关时序由 BoardIo_Update 推进。
 */
uint8_t BoardBuzzer_Play(uint16_t on_ms, uint16_t off_ms, uint8_t repeat_count);

/* 立即关闭蜂鸣器并清空全部待播放提示音。 */
void BoardBuzzer_Stop(void);

/* 当前正在鸣叫或队列仍有待播放模式时返回 1。 */
uint8_t BoardBuzzer_IsActive(void);

#endif
