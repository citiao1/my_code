/* USER CODE BEGIN Header */
/**
  * @file           : main.h
  * @brief          : Common application definitions for the VGT6 module test.
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define STATUS_LED_Pin GPIO_PIN_2
#define STATUS_LED_GPIO_Port GPIOE
#define BAT_VOLT_ADC_Pin GPIO_PIN_0
#define BAT_VOLT_ADC_GPIO_Port GPIOC
#define ENCODER2_A_Pin GPIO_PIN_0
#define ENCODER2_A_GPIO_Port GPIOA
#define ENCODER2_B_Pin GPIO_PIN_1
#define ENCODER2_B_GPIO_Port GPIOA
#define IMU_INT1_Pin GPIO_PIN_4
#define IMU_INT1_GPIO_Port GPIOC
#define IMU_CS_Pin GPIO_PIN_12
#define IMU_CS_GPIO_Port GPIOB
#define ENCODER3_A_Pin GPIO_PIN_14
#define ENCODER3_A_GPIO_Port GPIOB
#define ENCODER3_B_Pin GPIO_PIN_15
#define ENCODER3_B_GPIO_Port GPIOB
#define OLED_DC_Pin GPIO_PIN_11
#define OLED_DC_GPIO_Port GPIOD
#define OLED_RST_Pin GPIO_PIN_12
#define OLED_RST_GPIO_Port GPIOD
#define OLED_DIN_Pin GPIO_PIN_13
#define OLED_DIN_GPIO_Port GPIOD
#define OLED_CLK_Pin GPIO_PIN_14
#define OLED_CLK_GPIO_Port GPIOD
#define MOTOR1_PWM_Pin GPIO_PIN_6
#define MOTOR1_PWM_GPIO_Port GPIOC
#define MOTOR2_PWM_Pin GPIO_PIN_7
#define MOTOR2_PWM_GPIO_Port GPIOC
#define MOTOR3_PWM_Pin GPIO_PIN_8
#define MOTOR3_PWM_GPIO_Port GPIOC
#define MOTOR4_PWM_Pin GPIO_PIN_9
#define MOTOR4_PWM_GPIO_Port GPIOC
#define ENCODER1_A_Pin GPIO_PIN_8
#define ENCODER1_A_GPIO_Port GPIOA
#define ENCODER1_B_Pin GPIO_PIN_9
#define ENCODER1_B_GPIO_Port GPIOA
#define MOTOR1_DIR_Pin GPIO_PIN_0
#define MOTOR1_DIR_GPIO_Port GPIOD
#define MOTOR2_DIR_Pin GPIO_PIN_1
#define MOTOR2_DIR_GPIO_Port GPIOD
#define MOTOR3_DIR_Pin GPIO_PIN_2
#define MOTOR3_DIR_GPIO_Port GPIOD
#define MOTOR4_DIR_Pin GPIO_PIN_3
#define MOTOR4_DIR_GPIO_Port GPIOD
#define MOTOR1_BRAKE_Pin GPIO_PIN_4
#define MOTOR1_BRAKE_GPIO_Port GPIOD
#define MOTOR2_BRAKE_Pin GPIO_PIN_5
#define MOTOR2_BRAKE_GPIO_Port GPIOD
#define MOTOR3_BRAKE_Pin GPIO_PIN_6
#define MOTOR3_BRAKE_GPIO_Port GPIOD
#define MOTOR4_BRAKE_Pin GPIO_PIN_7
#define MOTOR4_BRAKE_GPIO_Port GPIOD
#define ENCODER4_A_Pin GPIO_PIN_6
#define ENCODER4_A_GPIO_Port GPIOB
#define ENCODER4_B_Pin GPIO_PIN_7
#define ENCODER4_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
