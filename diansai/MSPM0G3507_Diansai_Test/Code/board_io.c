#include "board_io.h"

#include "include.h"

#define BOARD_KEY_DEBOUNCE_MS       30U
#define BOARD_KEY_LONG_PRESS_MS    500U
#define BOARD_SWITCH_DEBOUNCE_MS    30U
#define BOARD_BUZZER_PIN            GPIO_Pin_A_28

/*
 * 板卡资料确认蜂鸣器连接 PA28，但没有标注有效电平。当前先按高电平有效；
 * 若上板测试发现逻辑相反，只需将此宏改为 0，不应改动提示音状态机。
 */
#define BOARD_BUZZER_ACTIVE_LEVEL    1U
#define BUZZER_QUEUE_SIZE            4U

typedef struct
{
    uint8_t raw_pressed;
    uint8_t stable_pressed;
    uint8_t long_reported;
    uint32_t raw_changed_ms;
    uint32_t pressed_since_ms;
} KeyState;

typedef struct
{
    uint8_t raw_down;
    uint8_t stable_down;
    uint32_t raw_changed_ms;
} SwitchState;

typedef struct
{
    uint16_t on_ms;
    uint16_t off_ms;
    uint8_t repeat_count;
} BuzzerPattern;

/*
 * 实车按键测试确认母板 K1/K2 与资料中的 PB14/PB15 顺序相反。
 * 在硬件抽象层交换前两项后，上层可以始终按 K1/K2 语义编写：
 * bit0=母板 K1/PB15，bit1=母板 K2/PB14，bit2=母板 K3/PB16。
 */
static const LQEnum_GPIO_Pin_t key_pins[BOARD_KEY_COUNT] = {
    GPIO_Pin_B_15,
    GPIO_Pin_B_14,
    GPIO_Pin_B_16,
};

/* 原理图：第一位拨码接 PB6，第二位拨码接 PB8，均由 2.2k 电阻上拉。 */
static const LQEnum_GPIO_Pin_t switch_pins[BOARD_SWITCH_COUNT] = {
    GPIO_Pin_B_6,
    GPIO_Pin_B_8,
};

static KeyState key_states[BOARD_KEY_COUNT];
static SwitchState switch_states[BOARD_SWITCH_COUNT];
static BoardKeyEvents pending_events;
static uint8_t pending_switch_changed_mask;
static BuzzerPattern buzzer_queue[BUZZER_QUEUE_SIZE];
static BuzzerPattern current_pattern;
static uint8_t buzzer_head;
static uint8_t buzzer_tail;
static uint8_t buzzer_active;
static uint8_t buzzer_phase_on;
static uint8_t buzzer_repeats_left;
static uint8_t buzzer_gap_active;
static uint8_t buzzer_continuous;
static uint32_t buzzer_deadline_ms;

static uint8_t ReadPressed(uint8_t index)
{
    /* 板载按键带上拉，按下时引脚接地，因此低电平表示按下。 */
    return LQ_GPIO_ReadPin(key_pins[index]) == 0 ? 1U : 0U;
}

static uint8_t ReadSwitchDown(uint8_t index)
{
    return LQ_GPIO_ReadPin(switch_pins[index]) == 0 ? 1U : 0U;
}

static uint8_t TimeReached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0 ? 1U : 0U;
}

static void WriteBuzzer(uint8_t on)
{
    int level = on ? (int)BOARD_BUZZER_ACTIVE_LEVEL :
                     (int)(1U - BOARD_BUZZER_ACTIVE_LEVEL);
    LQ_GPIO_WritePin(BOARD_BUZZER_PIN, level);
}

static uint8_t BuzzerQueueEmpty(void)
{
    return buzzer_head == buzzer_tail ? 1U : 0U;
}

static void StartNextBuzzerPattern(uint32_t now_ms)
{
    if (BuzzerQueueEmpty()) return;
    if (buzzer_gap_active)
    {
        if (!TimeReached(now_ms, buzzer_deadline_ms)) return;
        buzzer_gap_active = 0U;
    }

    current_pattern = buzzer_queue[buzzer_tail];
    buzzer_tail = (uint8_t)((buzzer_tail + 1U) % BUZZER_QUEUE_SIZE);
    buzzer_repeats_left = current_pattern.repeat_count;
    buzzer_active = 1U;
    buzzer_phase_on = 1U;
    WriteBuzzer(1U);
    buzzer_deadline_ms = now_ms + current_pattern.on_ms;
}

static void UpdateBuzzer(uint32_t now_ms)
{
    if (buzzer_continuous) return;
    if (!buzzer_active)
    {
        StartNextBuzzerPattern(now_ms);
        return;
    }
    if (!TimeReached(now_ms, buzzer_deadline_ms)) return;

    if (buzzer_phase_on)
    {
        WriteBuzzer(0U);
        if (buzzer_repeats_left > 0U) buzzer_repeats_left--;
        if (buzzer_repeats_left == 0U)
        {
            /*
             * 最后一声结束后仍保留 off_ms 间隔，再启动队列中的下一组提示音。
             * 否则“标定完成 1 声”和“归一化完成 3 声”会首尾相接，听起来
             * 像一声加长音，无法区分两个独立状态。
             */
            buzzer_active = 0U;
            buzzer_phase_on = 0U;
            buzzer_gap_active =
                (!BuzzerQueueEmpty() && current_pattern.off_ms > 0U) ? 1U : 0U;
            buzzer_deadline_ms = now_ms + current_pattern.off_ms;
        }
        else
        {
            buzzer_phase_on = 0U;
            buzzer_deadline_ms = now_ms + current_pattern.off_ms;
        }
    }
    else
    {
        buzzer_phase_on = 1U;
        WriteBuzzer(1U);
        buzzer_deadline_ms = now_ms + current_pattern.on_ms;
    }
}

void BoardIo_Init(uint32_t now_ms)
{
    uint8_t index;
    LQConfig_GPIO_InitTypeDef_t gpio = {
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_RESISTOR_PULL_UP,
        .Speed = GPIO_SPEED_LOW,
    };

    memset(key_states, 0, sizeof(key_states));
    memset(switch_states, 0, sizeof(switch_states));
    memset(&pending_events, 0, sizeof(pending_events));
    memset(buzzer_queue, 0, sizeof(buzzer_queue));
    buzzer_head = 0U;
    buzzer_tail = 0U;
    buzzer_active = 0U;
    buzzer_phase_on = 0U;
    buzzer_gap_active = 0U;
    buzzer_continuous = 0U;
    pending_switch_changed_mask = 0U;
    buzzer_deadline_ms = now_ms;

    /* 不调用 LQ_Key_Init：当前库版本把按键误配置成了推挽输出。 */
    for (index = 0U; index < BOARD_KEY_COUNT; index++)
    {
        LQ_GPIO_Init(key_pins[index], &gpio);
        key_states[index].raw_pressed = ReadPressed(index);
        key_states[index].stable_pressed = key_states[index].raw_pressed;
        key_states[index].raw_changed_ms = now_ms;
        key_states[index].pressed_since_ms = now_ms;
    }
    for (index = 0U; index < BOARD_SWITCH_COUNT; index++)
    {
        LQ_GPIO_Init(switch_pins[index], &gpio);
        switch_states[index].raw_down = ReadSwitchDown(index);
        switch_states[index].stable_down = switch_states[index].raw_down;
        switch_states[index].raw_changed_ms = now_ms;
    }

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_RESISTOR_NO_PULL;
    LQ_GPIO_Init(BOARD_BUZZER_PIN, &gpio);
    WriteBuzzer(0U);
}

void BoardIo_Update(uint32_t now_ms)
{
    uint8_t index;

    for (index = 0U; index < BOARD_KEY_COUNT; index++)
    {
        uint8_t raw = ReadPressed(index);
        uint8_t bit = (uint8_t)(1U << index);
        KeyState *key = &key_states[index];

        if (raw != key->raw_pressed)
        {
            key->raw_pressed = raw;
            key->raw_changed_ms = now_ms;
        }
        else if (raw != key->stable_pressed &&
                 now_ms - key->raw_changed_ms >= BOARD_KEY_DEBOUNCE_MS)
        {
            key->stable_pressed = raw;
            if (raw)
            {
                key->pressed_since_ms = now_ms;
                key->long_reported = 0U;
                pending_events.pressed_mask |= bit;
            }
            else
            {
                pending_events.released_mask |= bit;
                if (!key->long_reported) pending_events.short_press_mask |= bit;
            }
        }

        if (key->stable_pressed && !key->long_reported &&
            now_ms - key->pressed_since_ms >= BOARD_KEY_LONG_PRESS_MS)
        {
            key->long_reported = 1U;
            pending_events.long_press_mask |= bit;
        }
    }
    for (index = 0U; index < BOARD_SWITCH_COUNT; index++)
    {
        uint8_t raw = ReadSwitchDown(index);
        SwitchState *state = &switch_states[index];

        if (raw != state->raw_down)
        {
            state->raw_down = raw;
            state->raw_changed_ms = now_ms;
        }
        else if (raw != state->stable_down &&
                 now_ms - state->raw_changed_ms >= BOARD_SWITCH_DEBOUNCE_MS)
        {
            state->stable_down = raw;
            pending_switch_changed_mask |= (uint8_t)(1U << index);
        }
    }
    UpdateBuzzer(now_ms);
}

BoardKeyEvents BoardIo_TakeKeyEvents(void)
{
    BoardKeyEvents events = pending_events;
    memset(&pending_events, 0, sizeof(pending_events));
    return events;
}

uint8_t BoardIo_GetPressedMask(void)
{
    uint8_t index;
    uint8_t mask = 0U;

    for (index = 0U; index < BOARD_KEY_COUNT; index++)
    {
        if (key_states[index].stable_pressed) mask |= (uint8_t)(1U << index);
    }
    return mask;
}

uint8_t BoardIo_TakeSwitchChangedMask(void)
{
    uint8_t changed = pending_switch_changed_mask;
    pending_switch_changed_mask = 0U;
    return changed;
}

uint8_t BoardIo_GetSwitchDownMask(void)
{
    uint8_t index;
    uint8_t mask = 0U;

    for (index = 0U; index < BOARD_SWITCH_COUNT; index++)
    {
        if (switch_states[index].stable_down)
        {
            mask |= (uint8_t)(1U << index);
        }
    }
    return mask;
}

uint8_t BoardBuzzer_Play(uint16_t on_ms, uint16_t off_ms, uint8_t repeat_count)
{
    uint8_t next_head;

    if (on_ms == 0U || repeat_count == 0U || buzzer_continuous) return 0U;
    next_head = (uint8_t)((buzzer_head + 1U) % BUZZER_QUEUE_SIZE);
    if (next_head == buzzer_tail) return 0U;

    buzzer_queue[buzzer_head].on_ms = on_ms;
    buzzer_queue[buzzer_head].off_ms = off_ms;
    buzzer_queue[buzzer_head].repeat_count = repeat_count;
    buzzer_head = next_head;
    return 1U;
}

void BoardBuzzer_Stop(void)
{
    buzzer_head = 0U;
    buzzer_tail = 0U;
    buzzer_active = 0U;
    buzzer_phase_on = 0U;
    buzzer_gap_active = 0U;
    buzzer_continuous = 0U;
    WriteBuzzer(0U);
}

uint8_t BoardBuzzer_IsActive(void)
{
    return (buzzer_continuous || buzzer_active || !BuzzerQueueEmpty()) ? 1U : 0U;
}

void BoardBuzzer_SetContinuous(uint8_t enabled)
{
    if (enabled)
    {
        BoardBuzzer_Stop();
        buzzer_continuous = 1U;
        WriteBuzzer(1U);
    }
    else if (buzzer_continuous)
    {
        buzzer_continuous = 0U;
        WriteBuzzer(0U);
    }
}
