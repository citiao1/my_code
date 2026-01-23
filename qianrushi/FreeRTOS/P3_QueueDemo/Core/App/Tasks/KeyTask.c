#include "cmsis_os2.h"
#include "main.h"
#include "FreeRTOS.h"
#include "Types/LEDType.h"
#define IS_KEY_PRESSED() (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin)==GPIO_PIN_RESET)
#define KEY_CHECK_INTERVAL 10
#define KEY_DEBOUNCE_TIME 30
#define KEY_DEBOUNCE_COUNT (KEY_DEBOUNCE_TIME/KEY_CHECK_INTERVAL)
uint8_t isKey1Clicked() {
    static uint8_t count=0;
    static uint8_t pressed = 0;
    if (IS_KEY_PRESSED()&&!pressed) {
        count++;
        if (count>=KEY_DEBOUNCE_COUNT&&IS_KEY_PRESSED()) {
            pressed=1;
            return 1;
        }
    }
    if (!IS_KEY_PRESSED()) {
        pressed=0;
        count=0;
    }
    return 0;
}

void StartKeyTask(void *argument) {
    LEDState state=LEDState_Off;
    for (;;) {
        if (isKey1Clicked()) {
            state=!state;
            LEDMessage* message=pvPortMalloc(sizeof(LEDMessage));
            message->color = LEDColor_Red;
            message->state = state;
            osMessageQueuePut(LEDQueueHandle,(&message),0,osWaitForever);
        }
        osDelay(10);
    }
}