/* USER CODE BEGIN Header */
/**
  * @file           : main.h
  * @brief          : Common application definitions for the VGT6 module test.
  */
/* USER CODE END Header */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

#define IMU_CS_Pin              GPIO_PIN_12
#define IMU_CS_GPIO_Port        GPIOB
#define IMU_INT1_Pin            GPIO_PIN_4
#define IMU_INT1_GPIO_Port      GPIOC
#define OLED_DC_Pin             GPIO_PIN_11
#define OLED_DC_GPIO_Port       GPIOD
#define OLED_RST_Pin            GPIO_PIN_12
#define OLED_RST_GPIO_Port      GPIOD
#define OLED_DIN_Pin            GPIO_PIN_13
#define OLED_DIN_GPIO_Port      GPIOD
#define OLED_CLK_Pin            GPIO_PIN_14
#define OLED_CLK_GPIO_Port      GPIOD
#define STATUS_LED_Pin          GPIO_PIN_2
#define STATUS_LED_GPIO_Port    GPIOE

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
