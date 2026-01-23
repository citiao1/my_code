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
#include "adc.h"
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
#define UNLOCKLED       HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET)  //��
#define LOCKLED         HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
u32 uwTick_Lcd;
u32 uwTick_Key;
u32 uwTick_Adc;
u32 blink_timer;
uint8_t blink_state = 0;       
volatile uint8_t blink_mask = 0x01;
volatile uint8_t blink_mask1 = 0x02;
uint8_t enable_blink = 0; 
u8 KeyValue; 
u8 KeyState; 
int  count2=0;
int  count3=0;
int  count4=0;
int  LcdState=1;
int  select=1;
int upper_lcd=1;
int lower_lcd=2;
char LineString[1000];
char LineString1[1000];
char LineString2[1000];
char LineString3[1000];
char LineString4[1000];
char buffer[20];
char *text="Main";
char *text1="Setting";
char *upper="Status: Upper";
char *lower="Status: Lower";
double voltage=0;
double voltage_max=2.4;
double voltage_min=1.2;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Lcd_Pro(void);
void Key_Pro(void);
void lcd_middledisplay(u8 Line, char* sources);
void Adc_Pro(void);
void App_Low4Blink(int interval);
void LED_Disp(volatile uint8_t Led_num);
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
	LCD_Init();
	LCD_Clear(White);
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
			uint8_t blink_led = 0;
			Lcd_Pro();
			Key_Pro();
			Adc_Pro();
			
		
			
			if(voltage>voltage_max){
				App_Low4Blink(200);
				blink_led = blink_state ? blink_mask : 0; 
			}else if(voltage<voltage_min){
				App_Low4Blink(200);
				blink_led = blink_state ? blink_mask1 : 0; 
			}
			
				
				LED_Disp(blink_led);
			
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
void Lcd_Pro(void){
		if((uwTick-uwTick_Lcd)<200)return;
		uwTick_Lcd=uwTick;
		switch(LcdState){
			case 1:
				lcd_middledisplay(Line1, text);
				sprintf((char *)LineString,"Volt: %.2fV",voltage);
				lcd_middledisplay(Line3, LineString);
				if(voltage>voltage_max){lcd_middledisplay(Line4, upper);}
				else if(voltage<voltage_min){lcd_middledisplay(Line4, lower);}
				else{LCD_ClearLine(Line4);}
				break;
			case 2:
				
				
				switch (select){
					case 1:
						
						sprintf((char *)LineString1,"Volt: %.2fV",voltage_max);
						LCD_HighlightLineDefault(Line2, (char *)LineString1);
						lcd_middledisplay(Line1, text1);
						
						sprintf((char *)LineString,"Volt: %.2fV",voltage_min);
						lcd_middledisplay(Line3, LineString);
						sprintf((char *)LineString,"Upper: LD%d",upper_lcd);
						lcd_middledisplay(Line4, LineString);
						sprintf((char *)LineString,"Lower: LD%d",lower_lcd);
						lcd_middledisplay(Line5, LineString);
						break;
					case 2:
						
						sprintf((char *)LineString2,"Volt: %.2fV",voltage_min);
						LCD_HighlightLineDefault(Line3, (char *)LineString2);
						lcd_middledisplay(Line1, text1);
						sprintf((char *)LineString,"Volt: %.2fV",voltage_max);
						lcd_middledisplay(Line2, LineString);
						
						sprintf((char *)LineString,"Upper: LD%d",upper_lcd);
						lcd_middledisplay(Line4, LineString);
						sprintf((char *)LineString,"Lower: LD%d",lower_lcd);
						lcd_middledisplay(Line5, LineString);
								break;
					case 3:
						
						sprintf((char *)LineString3,"Upper: LD%d",upper_lcd);
						LCD_HighlightLineDefault(Line4, (char *)LineString3);
						lcd_middledisplay(Line1, text1);
						sprintf((char *)LineString,"Volt: %.2fV",voltage_max);
						lcd_middledisplay(Line2, LineString);
						sprintf((char *)LineString,"Volt: %.2fV",voltage_min);
						lcd_middledisplay(Line3, LineString);
						
						sprintf((char *)LineString,"Lower: LD%d",lower_lcd);
						lcd_middledisplay(Line5, LineString);
						break;
					case 4:
						
						sprintf((char *)LineString4,"Lower: LD%d",lower_lcd);
						LCD_HighlightLineDefault(Line5, (char *)LineString4);
						lcd_middledisplay(Line1, text1);
						sprintf((char *)LineString,"Volt: %.2fV",voltage_max);
						lcd_middledisplay(Line2, LineString);
						sprintf((char *)LineString,"Volt: %.2fV",voltage_min);
						lcd_middledisplay(Line3, LineString);
						sprintf((char *)LineString,"Upper: LD%d",upper_lcd);
						lcd_middledisplay(Line4, LineString);
						
						break;
					default:
						break;
				}
			break;
			default:
				break;
		}
}
void Key_Pro(void){
		if((uwTick-uwTick_Key)<10)return;
		uwTick_Key=uwTick;
		KeyValue= key_scanf();
		if(KeyValue==1){
			KeyState=1;
			
		}else if(KeyValue==2){
			KeyState=2;
		}else if(KeyValue==3){
			KeyState=3;
		}else if(KeyValue==4){
			KeyState=4;
		}
		switch(KeyState){
			case 1:
				LcdState++;
				LCD_Clear(White);
				if(LcdState>2)LcdState=1;
				KeyState=0;
				break;
			case 2:
				select++;
				
				if(select>4){
					select=1;
				}
				KeyState=0;
				break;
			case 3:
				if(select==1&&LcdState==2){
				voltage_max+=0.3;
				}
				if(select==2&&LcdState==2){
				voltage_min+=0.3;
				}
				if(select==3&&LcdState==2){
					upper_lcd++;
					blink_mask=blink_mask<<1;
					if(upper_lcd==lower_lcd){
						upper_lcd++;
						blink_mask=blink_mask<<1;
					}
				}
				if(select==4&&LcdState==2){
				lower_lcd++;
				blink_mask1=blink_mask1<<1;
				if(lower_lcd==upper_lcd){
						lower_lcd++;
						blink_mask1=blink_mask1<<1;
				}
				}
				if(voltage_max>3.3){
					voltage_max-=0.3;
				}
				if(voltage_min>3.3){
					voltage_min-=0.3;
				}
				if(upper_lcd>8){
					upper_lcd=1;
					blink_mask=0x01;
				}
				if(lower_lcd>8){
					lower_lcd=1;
					blink_mask1=0x01;
				}
				if(lower_lcd==upper_lcd){
						lower_lcd++;
						blink_mask1=blink_mask1<<1;
				}
				KeyState=0;
				break;
			case 4:
				if(select==1&&LcdState==2){
				voltage_max-=0.3;
				}
				if(select==2&&LcdState==2){
				voltage_min-=0.3;
				}
				if(select==3&&LcdState==2)
				{
				upper_lcd--;
				blink_mask=blink_mask>>1;
				if(upper_lcd==lower_lcd){
					upper_lcd--;
					blink_mask=blink_mask>>1;
				}
				}
				if(select==4&&LcdState==2)
				{
				lower_lcd--;
				blink_mask1=blink_mask1>>1;
				if(upper_lcd==lower_lcd){
					lower_lcd--;
					blink_mask1=blink_mask1>>1;
				}
				}
				if(lower_lcd<1){
				lower_lcd=8;
				blink_mask1=0x80;
				}
				if(upper_lcd<1){
				upper_lcd=8;
				blink_mask=0x80;
				}
				if(voltage_max<0){
					voltage_max+=0.3;
				}
				if(voltage_min<0){
					voltage_min+=0.3;
				}
				if(upper_lcd==lower_lcd){
					upper_lcd--;
					blink_mask=blink_mask>>1;
				}
				KeyState=0;
				break;
			default:
				break;
		}
}

void lcd_middledisplay(u8 Line, char* sources)
{
    int paddling = (20 - strlen(sources)) / 2;
    snprintf(buffer, 20, "%*s%s%*s", paddling, "", sources,paddling, "");
    LCD_DisplayStringLine(Line, (u8*)buffer);
}
void Adc_Pro(void){
		if((uwTick-uwTick_Adc)<200)return;
		uwTick_Adc=uwTick;
		HAL_ADC_Start(&hadc2);
		HAL_ADC_PollForConversion(&hadc2,1000);
		voltage=HAL_ADC_GetValue(&hadc2)*3.3/4096;
}
void App_Low4Blink(int interval)
{
    
    if (uwTick - blink_timer < interval) {
        return; 
    }
    blink_timer = uwTick;      
    blink_state ^= 1;
    

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
