#include <my_knob.h>
#include <sys/stat.h>
#define COUNTER_INIT_VALUE 65535/2
#define BTN_DEBOUNCE_TICKS 10
typedef enum {Pressed,Unpressed} BtnState;
void setCounter(int value) {
    __HAL_TIM_SET_COUNTER(&htim1,value);
}
void Knob_Init() {
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
    setCounter(COUNTER_INIT_VALUE);

}
KnobCallback onForwardCallback=NULL;
KnobCallback onBackwardCallback=NULL;
KnobCallback onPressedCallback=NULL;
void Knob_SetForwardCallback(KnobCallback callback) {
    onForwardCallback = callback;
}
void Knob_SetBackwardCallback(KnobCallback callback) {
    onBackwardCallback = callback;
}
void Knob_SetPressedCallback(KnobCallback callback) {
    onPressedCallback = callback;
}
uint32_t getCounter() {
    return __HAL_TIM_GET_COUNTER(&htim1);
}
BtnState getButtonState() {
    return HAL_GPIO_ReadPin(KnobBtn_GPIO_Port, KnobBtn_Pin)==GPIO_PIN_RESET?Pressed:Unpressed;
}
uint32_t getTick() {
    return HAL_GetTick();
}
void Knob_Loop() {
    uint32_t counter=getCounter();
    if (counter>COUNTER_INIT_VALUE) {
        if (onForwardCallback!=NULL) {
            onForwardCallback();
        }
    }else if (counter<COUNTER_INIT_VALUE) {
        if (onBackwardCallback!=NULL) {
            onBackwardCallback();
        }
    }
    setCounter(COUNTER_INIT_VALUE);
    BtnState btnState=getButtonState();
    static uint8_t callbackState=0;
    static uint32_t pressedTime=0;
    if (btnState==Pressed) {
        if (pressedTime==0) {
            pressedTime=getTick();
        }else if (callbackState==0&&getTick()-pressedTime>BTN_DEBOUNCE_TICKS) {
            if (onPressedCallback!=NULL) {
                onPressedCallback();
            }
            callbackState=1;
        }
    }else {
        pressedTime=0;
        callbackState=0;
    }
}