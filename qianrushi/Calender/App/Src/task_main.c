#include "task_main.h"
#define CURSOR_FLASH_INTERVAL 500
char weeks[7][10]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
uint8_t countingFlag=0;
uint32_t uwTick_Key=0;
uint8_t KeyValue=0;
uint32_t uwTick_Blink=0;
long long count=0;
uint8_t blink_state=0;
char *message="hello";
uint8_t timeout_flag=0;
uint32_t uwTick_Buzzer=0;
int pause_flag=0;
int count_flag=1;
typedef enum {
    CalenderState_Normal,
    CalenderState_Setting,
    CalenderState_Counting,
    CalenderState_StartCounting,
    CalenderState_Count,
}CalenderState;
typedef enum {Year,Month,Day,Hour,Minute,Second}SettingState;
typedef struct {uint8_t x1;uint8_t y1;uint8_t x2;uint8_t y2;}CursorPosition;
CursorPosition cursorPosition[6]={
    {24+0*8,17,24+4*8,17},
    {24+5*8,17,24+7*8,17},
    {24+8*8,17,24+10*8,17},
    {16+0*12,45,16+2*12,45},
    {16+3*12,45,16+5*12,45},
    {16+6*12,45,16+8*12,45},
};
CalenderState calenderState=CalenderState_Normal;
SettingState settingState=Year;
struct tm settingTime;
struct tm countingTime;
struct tm startCountingTime;
struct tm countTime;
SettingState countingState=Hour;
void onKnobForward() {
    if (calenderState==CalenderState_Setting) {
        switch (settingState) {
            case Year:
                settingTime.tm_year++;
                break;
            case Month:
                settingTime.tm_mon++;
                if (settingTime.tm_mon>11) {
                    settingTime.tm_mon=0;
                }
                break;
            case Day:
                settingTime.tm_mday++;
                if (settingTime.tm_mday>31) {
                    settingTime.tm_mday=1;
                }
                break;
            case Hour:
                settingTime.tm_hour++;
                if (settingTime.tm_hour>23) {
                    settingTime.tm_hour=0;
                }
                break;
            case Minute:
                settingTime.tm_min++;
                if (settingTime.tm_min>59) {
                    settingTime.tm_min=0;
                }
                break;
            case Second:
                settingTime.tm_sec++;
                if (settingTime.tm_sec>59) {
                    settingTime.tm_sec=0;
                }
                break;

        }
    }else {
        switch (countingState) {
            case Hour:
                countingTime.tm_hour++;
                if (countingTime.tm_hour>23) {
                    countingTime.tm_hour=0;
                }
                break;
            case Minute:
                countingTime.tm_min++;
                if (countingTime.tm_min>59) {
                    countingTime.tm_min=0;
                }
                break;
            case Second:
                countingTime.tm_sec++;
                if (countingTime.tm_sec>59) {
                    countingTime.tm_sec=0;
                }
                break;
            default:
                break;
        }
    }
}
void onKnobBackward() {
    if (calenderState==CalenderState_Setting) {
        switch (settingState) {
            case Year:
                settingTime.tm_year--;
                if (settingTime.tm_year<70) {
                    settingTime.tm_year=70;
                }
                break;
            case Month:
                settingTime.tm_mon--;
                if (settingTime.tm_mon<0) {
                    settingTime.tm_mon=11;
                }
                break;
            case Day:
                settingTime.tm_mday--;
                if (settingTime.tm_mday<0) {
                    settingTime.tm_mday=31;
                }
                break;
            case Hour:
                settingTime.tm_hour--;
                if (settingTime.tm_hour<0) {
                    settingTime.tm_hour=23;
                }
                break;
            case Minute:
                settingTime.tm_min--;
                if (settingTime.tm_min<0) {
                    settingTime.tm_min=59;
                }
                break;
            case Second:
                settingTime.tm_sec--;
                if (settingTime.tm_sec<0) {
                    settingTime.tm_sec=59;
                }
                break;

        }
    }else {
        switch (countingState) {
            case Hour:
                countingTime.tm_hour--;
                if (countingTime.tm_hour<0) {
                    countingTime.tm_hour=23;
                }
                break;
            case Minute:
                countingTime.tm_min--;
                if (countingTime.tm_min<0) {
                    countingTime.tm_min=59;
                }
                break;
            case Second:
                countingTime.tm_sec--;
                if (countingTime.tm_sec<0) {
                    countingTime.tm_sec=59;
                }
                break;
            default:
                break;
        }
    }
}
void onKnobPressed() {
    if (calenderState == CalenderState_Normal) {
        settingTime= *MY_RTC_GetTime();
        settingState=Year;
        calenderState=CalenderState_Setting;
    }else if (calenderState==CalenderState_Setting){
        if (settingState==Second) {
            MY_RTC_SetTime(&settingTime);
            calenderState=CalenderState_Normal;
        }else {
            settingState++;
        }
    }else if (calenderState==CalenderState_Counting) {

        if (countingState==Second) {
            __HAL_TIM_SET_COUNTER(&htim3,0);
            count=0;
            startCountingTime.tm_hour=countingTime.tm_hour;
            startCountingTime.tm_min=countingTime.tm_min;
            startCountingTime.tm_sec=countingTime.tm_sec;
            calenderState=CalenderState_StartCounting;
        }else {
            countingState++;
        }
    }
}
void blink() {
    if ((uwTick-uwTick_Blink)<200)return;
    uwTick_Blink=uwTick;
    if (blink_state==1) {
        HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_0);
    }
    if (calenderState==CalenderState_StartCounting) {
        if (pause_flag==1) {
            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,SET);
        }else {
            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,RESET);
        }
    }else if (calenderState==CalenderState_Count) {
        if (count_flag==1) {
            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,SET);
        }else {
            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,RESET);
        }
    }
}
void showTime(struct tm* time) {
    char str[80];
    sprintf(str,"%d-%02d-%02d",time->tm_year+1900,time->tm_mon+1,time->tm_mday);
    OLED_PrintASCIIString(24,0,str,&afont16x8,OLED_COLOR_NORMAL);
    sprintf(str,"%02d:%02d:%02d",time->tm_hour,time->tm_min,time->tm_sec);
    OLED_PrintASCIIString(16,20,str,&afont24x12,OLED_COLOR_NORMAL);
    char* week=weeks[time->tm_wday];
    uint8_t x_weeks=(128-(strlen(week)*8))/2;
    OLED_PrintASCIIString(x_weeks,48,week,&afont16x8,OLED_COLOR_NORMAL);
}
void showCursor() {
    static uint32_t startTime=0;
    uint32_t difftime=HAL_GetTick()-startTime;
    if (difftime>2*CURSOR_FLASH_INTERVAL) {
        startTime=HAL_GetTick();
    }else if ((difftime>CURSOR_FLASH_INTERVAL)&&calenderState==CalenderState_Setting) {
        CursorPosition position=cursorPosition[settingState];
        OLED_DrawLine(position.x1,position.y1,position.x2,position.y2,OLED_COLOR_NORMAL);
    }else if ((difftime>CURSOR_FLASH_INTERVAL)&&calenderState==CalenderState_Counting) {
        CursorPosition position=cursorPosition[countingState];
        OLED_DrawLine(position.x1,position.y1,position.x2,position.y2,OLED_COLOR_NORMAL);
    }

}
void key_pro() {
    if ((uwTick-uwTick_Key)<10)return;
    uwTick_Key=uwTick;
    KeyValue=key_scanf();
    if (KeyValue==1) {
        if (calenderState==CalenderState_Normal) {
            calenderState=CalenderState_Counting;
            struct tm* now=MY_RTC_GetTime();
            countingTime.tm_year=now->tm_year;
            countingTime.tm_mon=now->tm_mon;
            countingTime.tm_mday=now->tm_mday;
            countingTime.tm_wday=now->tm_wday;
            countingTime.tm_hour=0;
            countingTime.tm_min=0;
            countingTime.tm_sec=0;
            startCountingTime.tm_year=now->tm_year;
            startCountingTime.tm_mon=now->tm_mon;
            startCountingTime.tm_mday=now->tm_mday;
            startCountingTime.tm_wday=now->tm_wday;
            KeyValue=0;
        }else if (calenderState==CalenderState_StartCounting) {
            pause_flag^=1;
        }else if (calenderState==CalenderState_Count) {
            count_flag^=1;
        }

    }
    else if (KeyValue==2) {
        if (calenderState==CalenderState_Counting) {
            calenderState=CalenderState_Normal;
            timeout_flag=0;
            blink_state=0;
            HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,RESET);
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
        }else if (calenderState==CalenderState_Normal) {
            countTime.tm_hour=0;
            countTime.tm_min=0;
            countTime.tm_sec=0;
            struct tm* now=MY_RTC_GetTime();
            countTime.tm_year=now->tm_year;
            countTime.tm_mon=now->tm_mon;
            countTime.tm_mday=now->tm_mday;
            countTime.tm_wday=now->tm_wday;
            __HAL_TIM_SET_COUNTER(&htim3,0);
            count=0;
            calenderState=CalenderState_Count;
        }else if (calenderState==CalenderState_Count) {
            calenderState=CalenderState_Normal;
            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,RESET);
            count_flag=1;
        }

        KeyValue=0;
    }

}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim==&htim3) {
        if (((calenderState==CalenderState_StartCounting)&&(pause_flag==0))||(calenderState==CalenderState_Count&&count_flag==0)) {
            count++;

        }
    }
    if (timeout_flag==1) {
        calenderState=CalenderState_Counting;
        blink_state=1;
    }
    if (calenderState==CalenderState_StartCounting) {
        if (count%1000==0) {
            startCountingTime.tm_sec--;
            if (startCountingTime.tm_sec<0) {
                if (startCountingTime.tm_min>0||startCountingTime.tm_hour>0) {
                    startCountingTime.tm_sec=59;
                    startCountingTime.tm_min--;
                    if (startCountingTime.tm_min<0) {
                        startCountingTime.tm_hour--;
                        startCountingTime.tm_min=59;
                    }
                }else{timeout_flag=1;}
            }

        }
    }else if (calenderState==CalenderState_Count) {
        if ((count_flag==0)&&(count%1000==0)) {
            countTime.tm_sec++;
            if (countTime.tm_sec==60) {
                countTime.tm_sec=0;
                countTime.tm_min++;
                if (countTime.tm_min==60) {
                    countTime.tm_hour++;
                    countTime.tm_min=0;
                }
            }
        }

    }
}

void Buzzer() {
    if (blink_state==1) {
        if ((uwTick-uwTick_Buzzer)<200) {
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);

        }else {
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 500/5);
            uwTick_Buzzer=uwTick;
        }

    }
}
void MainTaskInit() {
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_4);
    OLED_Init();
    MY_RTC_Init();
    Knob_Init();
    Knob_SetForwardCallback(onKnobForward);
    Knob_SetBackwardCallback(onKnobBackward);
    Knob_SetPressedCallback(onKnobPressed);
}

void MainTask() {
    Knob_Loop();
    key_pro();
    OLED_NewFrame();
    blink();
    Buzzer();
    if (calenderState==CalenderState_Normal) {
        struct tm* now=MY_RTC_GetTime();
        showTime(now);
    }else if (calenderState==CalenderState_Setting) {
        showTime(&settingTime);
        showCursor();
    }else if (calenderState==CalenderState_Counting) {

        showTime(&countingTime);
        showCursor();
    }else if (calenderState==CalenderState_StartCounting) {
        blink_state=0;
        timeout_flag=0;
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,RESET);
        countingState=Hour;
        showTime(&startCountingTime);
    }else if (calenderState==CalenderState_Count) {
        showTime(&countTime);
    }

    OLED_ShowFrame();
}
