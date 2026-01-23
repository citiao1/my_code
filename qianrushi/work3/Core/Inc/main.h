/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
    
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;

    
extern uint8_t gucLed;
    
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define LEDFLOWLB 0X01
#define LEDFLOWLE 0X10			//左移结束位

#define LEDFLOWRB 0x08
#define LEDFLOWRE 0X00
#define KEY_STATE_0 0  // ????
#define KEY_STATE_1 1  // ???????
#define KEY_STATE_2 2//右移结束位
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define UNLOCKLED       HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET)  //高
#define LOCKLED         HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET)//低

void App_Low4Blink(int uwTickBlink);

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
