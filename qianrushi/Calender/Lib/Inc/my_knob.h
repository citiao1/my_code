//
// Created by 123 on 2025/12/4.
//

#ifndef CALENDER_MY_KNOB_H
#define CALENDER_MY_KNOB_H
#include "tim.h"
typedef void(*KnobCallback)(void);
void Knob_Init();
void Knob_Loop();
void Knob_SetForwardCallback(KnobCallback callback);
void Knob_SetBackwardCallback(KnobCallback callback);
void Knob_SetPressedCallback(KnobCallback callback);
#endif //CALENDER_MY_KNOB_H