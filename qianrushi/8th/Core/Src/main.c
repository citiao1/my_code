/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rtc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define led_left 0x10
#define led_right 0x00
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UNLOCK HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
#define LOCK HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
#define UP HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
#define DOWN HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
#define OPEN HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET);
#define CLOSE HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
u32 uwTick_led=0;
u8 ucLed=0x08;
u8 lcd_state=1;
u32 uwTick_key=0;
u32 uwTick_lcd=0;
char *title="current state";
char message[1000];
u8 total_led=0;
u8 lcd_led=0;
u8 set_mode=1;
u8 key_value=0;
u32 count=0;
u8 floor1=0;
u8 floor2=0;
u8 floor3=0;
u8 floor4=0;
u32 uwTick_floor=0;
u8 floor_state=1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void led_disp(u8 led){
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15,GPIO_PIN_SET);
  UNLOCK;
  LOCK;
  HAL_GPIO_WritePin(GPIOC,led<<8,GPIO_PIN_RESET);
  UNLOCK;
  LOCK;
}
u8 flowkleft(u8 led){
  if(uwTick-uwTick_led<200)return led;
  uwTick_led=uwTick;
  led=led<<1;
  if(led==led_right)led=led_left;
  return led;
}
u8 flowkright(u8 led){
  if(uwTick-uwTick_led<200)return led;
  uwTick_led=uwTick;
  led=led>>1;
  if(led==led_left){led=led_right;}
  return led;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
  if(htim==&htim7){
    if(key_value==0){
      count++;
    }else{
      count=0;
    }
    if(count>=1000&&set_mode==1){
      set_mode^=1;
      CLOSE
      count=0;
      
    }
  }
}
void key_pro(){
  if(uwTick-uwTick_key<10)return;
  uwTick_key=uwTick;
  key_value=key_scanf();
  if(key_value==1){
    lcd_state=1;
    floor1=1;
    lcd_led=0x01;
    LCD_Clear(White);
  }
  if(key_value==2){
    lcd_state=2;
    lcd_led=0x02;
    floor2=1;
    LCD_Clear(White);
  }
  if(key_value==3){
    lcd_state=3;
    lcd_led=0x04;
    floor3=1;
    LCD_Clear(White);
  }
  if(key_value==4){
    lcd_state=4;
    lcd_led=0x08;
    floor4=1;
    LCD_Clear(White);
  }

}
void lcd_pro(){
  if(uwTick-uwTick_lcd<100)return;
  uwTick_lcd=uwTick;
  LCD_middle_show_string(Line1,title);
  sprintf((char *)message,"%d",lcd_state);
  LCD_middle_show_string(Line3,message);
}
int UP_DOWN_delay(){
  if(uwTick-uwTick_floor<6000)return 1;
  uwTick_floor=uwTick;
  return 0;
}
int OPEN_CLOSE_delay(){
  if(uwTick-uwTick_floor<4000)return 1;
  uwTick_floor=uwTick;
  return 0;
}
int FLOOR_delay(){
  if(uwTick-uwTick_floor<2000)return 1;
  uwTick_floor=uwTick;
  return 0;
} 
void floor_pro(){
  
  switch (floor_state)
  {
    case 1:
      if(floor2==1){
        UP
        if(UP_DOWN_delay())return;
        lcd_state=2;
        lcd_led=0x02;
        OPEN 
        if(OPEN_CLOSE_delay())return;
        floor2=0;
      }
      if(floor3==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        UP
        if(UP_DOWN_delay())return;
        lcd_state=3;
        lcd_led=0x04;
        if(OPEN_CLOSE_delay())return;
        OPEN
        floor3=0;
      }
      if(floor4==1){
        if(FLOOR_delay())return;
        if(OPEN_CLOSE_delay())return;
        CLOSE
        UP
        if(UP_DOWN_delay())return;
        lcd_state=4;
        lcd_led=0x08;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor4=0;
      }
    case 2:
      if(floor3==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        UP
        if(UP_DOWN_delay())return;
        lcd_state=3;
        lcd_led=0x04;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor3=0;
      }
      if(floor4==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        UP
        if(UP_DOWN_delay())return;
        lcd_state=4;
        lcd_led=0x08;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor4=0;
      }
      if(floor1==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        DOWN
        if(UP_DOWN_delay())return;
        lcd_state=1;
        lcd_led=0x01;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor1=0;
      }
    case 3:
      if(floor3==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        UP
        if(UP_DOWN_delay())return;
        lcd_state=3;
        lcd_led=0x04;
        OPEN
        floor3=0;
      }
      if(floor2==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        DOWN
        if(UP_DOWN_delay())return;
        lcd_state=2;
        lcd_led=0x02;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor2=0;
      }
      if(floor1==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        DOWN
        if(UP_DOWN_delay())return;
        lcd_state=1;
        lcd_led=0x01;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor1=0;
      }
    case 4:
      if(floor3==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        DOWN
        if(UP_DOWN_delay())return;
        lcd_state=3;
        lcd_led=0x04;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor3=0;
      }
      if(floor2==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        DOWN
        if(UP_DOWN_delay())return;
        lcd_state=2;
        lcd_led=0x02;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor2=0;
      }
      if(floor1==1){
        if(FLOOR_delay())return;
        CLOSE
        if(OPEN_CLOSE_delay())return;
        DOWN
        if(UP_DOWN_delay())return;
        lcd_state=1;
        lcd_led=0x01;
        OPEN
        if(OPEN_CLOSE_delay())return;
        floor1=0;
      }
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init(); 
  LCD_Clear(White);
  LCD_SetTextColor(Black);
  LCD_SetBackColor(White);
  HAL_TIM_Base_Start_IT(&htim7);
  led_disp(0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if(set_mode){
      key_pro();
    }else{

    }
    lcd_pro();
    total_led=lcd_led|ucLed;
    led_disp(total_led);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV3;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
