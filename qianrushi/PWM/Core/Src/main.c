/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "string.h"
#include "stdio.h"
#include  <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UNLOCKLED       HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET)  
#define LOCKLED         HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET)
#define LEDFLOWLB 0X01
#define LEDFLOWLE 0X80
#define LEDFLOWRB 0x08
#define LEDFLOWRE 0X01
/* 修改 main.c 顶部的宏定义 */
#define GRAPH_HIGH_Y  160   // 高电平显示的行位置 (Xpos)
#define GRAPH_LOW_Y   210   // 低电平显示的行位置 (Xpos)
#define CYCLE_WIDTH   80    // 一个周期在屏幕上占的宽度 (像素)  // ★ 关键修改：周期改小一点，让屏幕能多显示几个波

// 保存上一次的占空比，用于防闪烁判断

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
u32 uwTick_Key=0;
u8 KeyValue; 
u8 KeyState; 
u8 ucLed=0x01;
u32 uwTick_Lcd=0;
char LineString[1000];
char buffer[20];
uint8_t blink_state = 0;  
volatile uint8_t blink_mask = 0x01;
u16  PWMValue;
int count=0;
u32 fre,capture_value;
u32 ic_period=0;
u32 ic_pulse=0;
float duty_cycle=0.0;
float frequency=0.0;
u32 uwTick_ic=0;
u32 pwm_freq=1000;
float last_drawn_duty = -1.0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Key_Pro(void){
		if((uwTick-uwTick_Key)<10)return;
		uwTick_Key=uwTick;
		KeyValue=key_scanf();
		if(KeyValue==1){
			KeyState=1;
		}else if(KeyValue==2){
			KeyState=2;
		}else if(KeyValue==3){
			KeyState=3;
		}else if(KeyValue==4){
			KeyState=4;
		}
		if(KeyState==1){			 
		PWMValue +=5; 
		if(PWMValue >= 100)PWMValue =100; 
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, PWMValue); 
		KeyState=0;
		}
		else if(KeyState==2){
			if(PWMValue<=0)PWMValue+=5;
			PWMValue-=5;
			__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,PWMValue);
			KeyState=0;
		}else if(KeyState==3){
      pwm_freq+=100;
      if(pwm_freq>=20000)pwm_freq=20000;
      u32 new_psc=(80000000/(pwm_freq*100))-1;
      __HAL_TIM_SET_PRESCALER(&htim2,new_psc);
      KeyState=0;
    }else if(KeyState==4){
      pwm_freq-=100;
      if(pwm_freq<=100)pwm_freq=100;
      u32 new_psc=(80000000/(pwm_freq*100))-1;
      __HAL_TIM_SET_PRESCALER(&htim2,new_psc);
      KeyState=0;
    }
    
    else{
			KeyState=0;
		}
	}
void LED_Disp(volatile uint8_t Led_num)
{
   
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12,GPIO_PIN_SET);
    UNLOCKLED;
    LOCKLED;
    
    HAL_GPIO_WritePin(GPIOC,Led_num<<8,GPIO_PIN_RESET);
    UNLOCKLED;
    LOCKLED;
}

void App_Low4Blink()
{        
    blink_state ^= 1;
}

/* 这里是配合上面参数的 Draw_Wave 实现 */
void Draw_Wave(void)
{
    // 1. 先清除绘图区域
    LCD_SetBackColor(White);
    LCD_ClearLine(Line6);
    LCD_ClearLine(Line7);
    LCD_ClearLine(Line8);
    LCD_ClearLine(Line9);
    
    // 2. 设置波形颜色
    LCD_SetTextColor(Blue);

    // 3. 计算高低电平的像素宽度
    int high_width = (int)(CYCLE_WIDTH * duty_cycle / 100.0f);
    int low_width = CYCLE_WIDTH - high_width;
    int row_height = 8;  // 波形线条的高度（厚度）

    // 4. 循环绘制波形
    int cur_y = 20;
    
    while(cur_y < 300)
    {
        // --- 特殊情况：0% 或 100% 占空比 ---
        if(duty_cycle >= 99.9f) {
            // 全是高电平 - 填充矩形
            for(int x = GRAPH_HIGH_Y; x < GRAPH_HIGH_Y + row_height; x++) {
                LCD_SetCursor(x, cur_y);
                LCD_WriteRAM_Prepare();
                for(int y = 0; y < CYCLE_WIDTH; y++) {
                    LCD_WriteRAM(Blue);
                }
            }
            cur_y += CYCLE_WIDTH;
            continue;
        }
        if(duty_cycle <= 0.1f) {
            // 全是低电平 - 填充矩形
            for(int x = GRAPH_LOW_Y; x < GRAPH_LOW_Y + row_height; x++) {
                LCD_SetCursor(x, cur_y);
                LCD_WriteRAM_Prepare();
                for(int y = 0; y < CYCLE_WIDTH; y++) {
                    LCD_WriteRAM(Blue);
                }
            }
            cur_y += CYCLE_WIDTH;
            continue;
        }

        // --- 正常方波 ---
        
        // A. 画高电平矩形（填充）
        if(high_width > 0) {
            for(int x = GRAPH_HIGH_Y; x < GRAPH_HIGH_Y + row_height; x++) {
                LCD_SetCursor(x, cur_y);
                LCD_WriteRAM_Prepare();
                for(int y = 0; y < high_width; y++) {
                    LCD_WriteRAM(Blue);
                }
            }
        }

        // B. 画低电平矩形（填充）
        if(low_width > 0) {
            for(int x = GRAPH_LOW_Y; x < GRAPH_LOW_Y + row_height; x++) {
                LCD_SetCursor(x, cur_y + high_width);
                LCD_WriteRAM_Prepare();
                for(int y = 0; y < low_width; y++) {
                    LCD_WriteRAM(Blue);
                }
            }
        }
        
        cur_y += CYCLE_WIDTH;
    }
    
    // 恢复文字颜色
    LCD_SetTextColor(Black);
}

void lcd_middledisplay(u8 Line, char* sources)
{
    int paddling = (20 - strlen(sources)) / 2;
    snprintf(buffer, 20, "%*s%s%*s", paddling, "", sources,paddling, "");
    LCD_DisplayStringLine(Line, (u8*)buffer);
}
void Lcd_Pro(void){
		if((uwTick-uwTick_Lcd)<200)return;
		uwTick_Lcd=uwTick;
    if(uwTick-uwTick_ic>50){
      if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_11)==GPIO_PIN_SET){
        duty_cycle=100.0f;
      }else{
        duty_cycle=0.0f;
      }
      frequency=0.0f;
    }
		sprintf((char *)LineString,"PWM_duty: %d",PWMValue);
		lcd_middledisplay(Line3, LineString);
    sprintf((char *)LineString,"count: %.2f",duty_cycle);
    lcd_middledisplay(Line4, LineString);
    sprintf((char *)LineString,"frequency: %.2f",frequency);
    lcd_middledisplay(Line5, LineString);
    Draw_Wave();

}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim){
		if(htim->Instance==TIM17){
				capture_value=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_1);
				TIM17->CNT = 0;
				if(capture_value > 0) {
            fre = 80000000 / (80 * capture_value);
        }
		}
    if(htim->Instance==TIM4&&htim->Channel==HAL_TIM_ACTIVE_CHANNEL_1){
      uwTick_ic=uwTick;
      ic_period=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_1);
      ic_pulse=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_2);
      if(ic_period>0){
        duty_cycle=((float)ic_pulse/ic_period)*100.0f;
        frequency=1000000.0f/ic_period;
      }
    }

}



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
//Pulse ratio
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */


		

	LCD_Init();
	LCD_Clear(White);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM17_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	LED_Disp(0);
  HAL_TIM_IC_Start_IT(&htim17,TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim4,TIM_CHANNEL_1);
  HAL_TIM_IC_Start(&htim4,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
	__HAL_TIM_CLEAR_IT(&htim2,TIM_IT_UPDATE); 
	HAL_TIM_IC_Start_IT(&htim17,TIM_CHANNEL_1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    
    Lcd_Pro();
		Key_Pro();
		
		
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
