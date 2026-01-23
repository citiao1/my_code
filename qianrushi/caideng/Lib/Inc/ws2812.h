
#ifndef CAIDENG_WS2812_H
#define CAIDENG_WS2812_H
#include "tim.h"
#define LED_COUNT 10
void WS2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);

void WS2812_Update();
#endif //CAIDENG_WS2812_H