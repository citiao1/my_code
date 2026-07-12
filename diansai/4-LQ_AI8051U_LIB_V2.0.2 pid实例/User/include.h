#ifndef __APPINCLUDE_H
#define __APPINCLUDE_H


// 在此处添加用STC_Driver头文件，省去在每个需要的文件前一个一个添加的繁琐

#include "config.h"
#include "AI8051U_Delay.h"
#include "AI8051U_clock.h"
#include "AI8051U_Switch.h"
#include "AI8051U_GPIO.h"
#include "AI8051U_WDT.h"
#include "AI8051U_NVIC.h"
#include "AI8051U_UART.h"
#include "AI8051U_DMA.h"
#include "AI8051U_Exti.h"
#include "AI8051U_ADC.h"
#include "AI8051U_PWM.h"
#include "AI8051U_Timer.h"
#include "AI8051U_I2C.h"
#include "AI8051U_SPI.h"
#include "AI8051U_EEPROM.h"


// 在此处添加用户头文件引用，省去在每个需要的文件前一个一个添加的繁琐

#include "LQ_GPIO_LED.h"
#include "LQ_WDT.h"
#include "LQ_KEY.h"
#include "LQ_ADC_KEY.h"
#include "LQ_UART.h"
#include "LQ_OLED096.h"
#include "LQ_IPSLCD.h"
#include "LQ_XINT.h"
#include "LQ_ADC.h"
#include "LQ_TIMER.h"
#include "LQ_MotorServo.h"
#include "LQ_Encoder.h"
#include "LQ_EEPROM.h"
#include "LQ_Hard_I2C.h"
#include "LQ_Soft_I2C.h"
#include "LQ_Soft_SPI.h"
#include "LQ_I2C_VL53.h"
#include "LQ_LSM6DSR_Soft.h"
#include "LQ_LSM6DSR_Hard.h"
#include "LQ_HI2C_Gyro.h"
#include "LQ_BDGPS.h"
#include "LQ_UTM.h"
#include "LQ_SBUS.h"
#include "LQ_CCD.h"

/*添加by zhuang消除了警告*/
#include "lcd.h"
#include "chassis.h"
#endif

