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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define UNLOCKLED       HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET)  
#define LOCKLED         HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET)
#define LEDFLOWLB 0X01
#define LEDFLOWLE 0X10
#define LEDFLOWRB 0x10
#define LEDFLOWRE 0X01
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
u8 ucLed=0x01;
u32 uwTick_Lcd=0;
char LineString[1000];
char buffer[20];
uint8_t blink_state = 0;  
volatile uint8_t blink_mask = 0x40|0x80;
u32 uwTick_LED_Speed_nms=0;
uint32_t flow_interval = 300;  
int left_flag=1;
int right_flag=0;
u32 uwTick_Key=0;
u8 KeyValue; 
u8 KeyState; 
u32 blink_timer;
char *text="Timer";
char *working="1Car on Working";
char *charging="2Car on Charging";
char *state_normal="BarrState:Normal";
char *state_lower="BarrState:Lower";
char *state_charging="BarrState:Charging";
char *state_fully="BarrState:Fully";
char *state_set="Status: Setting";
char LineString1[1000];
char LineString2[1000];
char HelloStr[] = "Hello world";
char bar_count0[]="ok";
char LineString4[1000];
u8 working_state=1;
u8 charging_state=0;
u32 uwTick_Adc;
double voltage;
double barr_low=1.0;
int bar_count=0;
u32 uwTick_bar;
u8 low_state;
u8 mag_state=0;
static uint8_t gLedBitL;

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
void App_Low4Blink(int interval)
{        
  if (uwTick - blink_timer < interval) {
        return; 
    }
    blink_timer = uwTick;      
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
		if(working_state==1){
			
			lcd_middledisplay(Line1, working);
			sprintf((char *)LineString2,"BarrVolt %.1fV",voltage);
			lcd_middledisplay(Line5, LineString2);
			if(voltage<barr_low){
				lcd_middledisplay(Line4, state_lower);
			}else{
				lcd_middledisplay(Line4, state_normal);
			}
		}
		else if(working_state==0){
			
			lcd_middledisplay(Line1,charging);
			sprintf((char *)LineString2,"BarrVolt %.1fV",voltage);
			lcd_middledisplay(Line3, LineString2);
			if(voltage>=3.0){
				lcd_middledisplay(Line4, state_fully);
			}else{
				lcd_middledisplay(Line4, state_charging);
			}
			if(mag_state==0){
				sprintf((char *)LineString2,"ChargCycles: %d",bar_count);
				LCD_HighlightLineDefault(Line6, (char *)LineString2);
				sprintf((char *)LineString2,"BarrMin %.1fV",barr_low);
				lcd_middledisplay(Line7, LineString2);
			}
			else if(mag_state==1){
					sprintf((char *)LineString2,"ChargCycles: %d",bar_count);
					lcd_middledisplay(Line6, LineString2);
					sprintf((char *)LineString2,"BarrMin %.1fV",barr_low);
					LCD_HighlightLineDefault(Line7, (char *)LineString2);
			}
			
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
		if(KeyState==1){
			if(working_state==0&&mag_state==0){
				bar_count=0;
			}else if(working_state==0&&mag_state==1){
				barr_low+=0.1;
				if(barr_low>1)barr_low-=0.1;
			}
			KeyState=0;
		}else if(KeyState==2){
			if(working_state==0&&mag_state==1){
			barr_low-=0.1;
			if(barr_low<0.1)barr_low+=0.1;
			}
			KeyState=0;
		}else if(KeyState==3){
			if(working_state==0)mag_state^=1;
			KeyState=0;
		}else if(KeyState==4){
			working_state^=1;
			
			LCD_Clear(Blue);
			
			
			KeyState=0;
		}
}
void Charging_pro(void){
		if((uwTick-uwTick_bar)<10)return;
		uwTick_bar=uwTick;
		if(voltage<barr_low){
				low_state=1;
		}
		if(low_state==1&&(voltage>3.0)){
			bar_count++;
			low_state=0;
		}
}
void Rx_Process(void)
{
     if(Rx_flg ==1)
      {
        if(working_state==0&&(!strcmp((char *)Rx_Buff,"ChargeCycles0\r\n"))){
					bar_count=0;
					HAL_UART_Transmit(&huart1, (u8*)bar_count0, strlen(bar_count0), 0xffff);
				}
				else if(!strcmp((char *)Rx_Buff,"BarrVolt\r\n")){
					sprintf((char *)LineString4,"BarrVolt: %.1fV\r\n",voltage);
					HAL_UART_Transmit(&huart1, (u8*)LineString4, strlen(LineString4), 0xffff);
				}
				
        
						memset(Rx_Buff,0x00, 256);
            Rx_cnt =0;
            Rx_flg =0;
}
}
void Adc_Pro(void){
		if((uwTick-uwTick_Adc)<200)return;
		uwTick_Adc=uwTick;
		HAL_ADC_Start(&hadc2);
		HAL_ADC_PollForConversion(&hadc2,1000);
		voltage=HAL_ADC_GetValue(&hadc2)*3.3/4096;
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
gLedBitL = LEDFLOWLB; 

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
  MX_ADC2_Init();
  MX_TIM7_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart1,&aRX,1);
	LCD_Init();
	LCD_Clear(Blue);
	LCD_SetTextColor(White);
	LCD_SetBackColor(Blue);
	LED_Disp(0);
	memset(Rx_Buff, 0x00, sizeof(Rx_Buff));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		u8 blink_led=0;
		u8 flow_led=0;
		u8 total_led=0;
		u8 up_led=0;
		Rx_Process();
		Lcd_Pro();
		Key_Pro();
		Adc_Pro();
		Charging_pro();
		if(working_state==1&&(voltage<barr_low)){
			App_Low4Blink(200);
			blink_led = blink_state ? blink_mask : 0; 
		}else if(working_state==1&&!(voltage<barr_low)){
			up_led=0x40|0x80;
		}
		else if(working_state==0&&(voltage<3.0)){
			App_Low4Blink(200);
			blink_led = blink_state ? blink_mask : 0; 
			gLedBitL = FlowLedLeft_app(gLedBitL);
      flow_led = gLedBitL;
		}
		else if(working_state==0&&(voltage>=3.0)){
			up_led=0x40|0x80;
		}
			
		total_led=flow_led|blink_led|up_led;
		LED_Disp(total_led);
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
