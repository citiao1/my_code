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
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include <stdio.h>
#include "string.h"
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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
long long count=0;
int count_time=0;
u8 ucLed=0x01;
u32 uwTick_Lcd=0;
char LineString[1000];
char buffer[20];
uint8_t blink_state = 0;  
volatile uint8_t blink_mask = 0x01;
u32 uwTick_LED_Speed_nms=0;
uint32_t flow_interval = 500;  
int left_flag=1;
int right_flag=0;
u32 uwTick_Key=0;
u8 KeyValue; 
u8 KeyState; 
long long time_keep=0;
u8 timer_state=1;
int time=0;
char *text="Timer";
char *state_run="Status: Running";
char *state_stop="Status: time out";
char *state_set="Status: Setting";
char LineString1[1000];
char LineString2[1000];
u8 set_state=1;
u8 countdown_state;
RTC_TimeTypeDef H_M_S_Time;
RTC_DateTypeDef Y_M_D_Date;
char HelloStr[] = "Hello world\r\n";
char time_plus[]="time plus\r\n";
char time_reduce[]="time reduce\r\n";
char time_start[]="time start\r\n";
char time_out[]="time out\r\n";
char time_set[]="set state\r\n";
char LineString4[1000];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
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

void lcd_middledisplay(u8 Line, char* sources)
{
    int paddling = (20 - strlen(sources)) / 2;
    snprintf(buffer, 20, "%*s%s%*s", paddling, "", sources,paddling, "");
    LCD_DisplayStringLine(Line, (u8*)buffer);
}
void Lcd_Pro(void){
		if((uwTick-uwTick_Lcd)<200)return;
		uwTick_Lcd=uwTick;
		HAL_RTC_GetTime(&hrtc,&H_M_S_Time,RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc,&Y_M_D_Date,RTC_FORMAT_BIN);
		lcd_middledisplay(Line1, text);
		sprintf((char *)LineString2,"Time: %dS",time);
		lcd_middledisplay(Line3, LineString2);
		sprintf((char *)LineString2,"%d:%d:%d",H_M_S_Time.Hours,H_M_S_Time.Minutes,H_M_S_Time.Seconds);
		lcd_middledisplay(Line8, LineString2);
		sprintf((char *)LineString2,"%d:%d:%d",Y_M_D_Date.Year,Y_M_D_Date.Month,Y_M_D_Date.Date);
		lcd_middledisplay(Line9, LineString2);
		if(set_state){
			lcd_middledisplay(Line4, state_set);
			App_Low4Blink();
			ucLed = blink_state ? blink_mask : 0; 
			LED_Disp(ucLed);
		}else if(countdown_state){
			lcd_middledisplay(Line4, state_run);
		}else{
			lcd_middledisplay(Line4, state_stop);
		}
		
}
uint8_t FlowLedLeft_app(volatile uint8_t Led_Bit)
{
    
    
    if((uwTick - uwTick_LED_Speed_nms)<flow_interval)   {return Led_Bit;}
        uwTick_LED_Speed_nms = uwTick; 
    
    LED_Disp(Led_Bit);  

    Led_Bit = Led_Bit<<1;  
    if(Led_Bit == LEDFLOWLE)   Led_Bit =LEDFLOWLB;
    return  Led_Bit;

}
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
		if(countdown_state==0&&KeyState==1){
		LED_Disp(0);
		countdown_state=1;
		set_state=0;
		time_keep=count;
		KeyState=0;
		sprintf((char *)LineString4,"Time: %dS\r\n",time);
		HAL_UART_Transmit(&huart1, (u8*)LineString4, strlen(LineString4), 0xffff);
		}else if(set_state&&KeyState==2){
		time++;
		KeyState=0;
		}else if(set_state&&KeyState==3){
		time--;
		KeyState=0;
		if(time<0){
			time++;
		}
		KeyState=0;
	}else if((countdown_state==0)&&(set_state==0)&&(KeyState==4)){
		set_state=1;
		KeyState=0;
	}else{
		KeyState=0;
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  
	if(htim==&htim7){
		count++;
	}
	if(count%1000==0){
		count_time++;
	}
	if(time<0){
		time=0;
		countdown_state=0;
		set_state=0;
		HAL_UART_Transmit(&huart1, (u8*)time_out, strlen(time_out), 0xffff);
	}
	if(countdown_state){
		if(count-time_keep>=1000){
			time_keep=count;
			time--;
			if(time>0){sprintf((char *)LineString4,"Time: %dS\r\n",time);
			HAL_UART_Transmit(&huart1, (u8*)LineString4, strlen(LineString4), 0xffff);}
		}
		
	}


}
void Rx_Process(void)
{
     if(Rx_flg ==1)
      {
        if(set_state==1&&(!strcmp((char *)Rx_Buff,"plus\r\n"))){
					time++;
					HAL_UART_Transmit(&huart1, (u8*)time_plus, strlen(time_plus), 0xffff);
				}
				else if(set_state==1&&(!strcmp((char *)Rx_Buff,"reduce\r\n"))){
					time--;
					HAL_UART_Transmit(&huart1, (u8*)time_reduce, strlen(time_reduce), 0xffff);
				}
				else if(set_state==1&&(!strcmp((char *)Rx_Buff,"start\r\n"))){
					countdown_state=1;
					set_state=0;
					time_keep=count;
					HAL_UART_Transmit(&huart1, (u8*)time_start, strlen(time_start), 0xffff);
					sprintf((char *)LineString4,"Time: %dS\r\n",time);
					HAL_UART_Transmit(&huart1, (u8*)LineString4, strlen(LineString4), 0xffff);
				}
				else if(set_state==0&&countdown_state==0&&(!strcmp((char *)Rx_Buff,"set\r\n"))){
					set_state=1;
					HAL_UART_Transmit(&huart1, (u8*)time_set, strlen(time_set), 0xffff);
				}
        
						memset(Rx_Buff,0x00, 256);
            Rx_cnt =0;
            Rx_flg =0;
      
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
  MX_TIM7_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart1,&aRX,1);
	HAL_TIM_Base_Start_IT(&htim7);
	LCD_Init();
	LCD_Clear(White);
	LED_Disp(0);
	memset(Rx_Buff, 0x00, sizeof(Rx_Buff));
	HAL_UART_Transmit(&huart1,(u8 *) HelloStr, strlen(HelloStr), 0xffff);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Rx_Process();
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
