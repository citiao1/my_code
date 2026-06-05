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
#include "adc.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
u32 key_time=0;
u32 lcd_time=0;
u8 lcd_state=0;
u8 key_value=0;
u8 key_state=0;
RTC_TimeTypeDef H_M_S_Time;
RTC_DateTypeDef Y_M_D_Time;
RTC_TimeTypeDef H_M_S_setTime;
RTC_DateTypeDef Y_M_D_setTime;
double uart_fre=0;
int uart_duty=0;
int uart_time=0;
char title1[30]="       IDLE   ";
char title2[30]="       PARA   ";
char title3[30]="       OUTP   ";
char title4[30]="       RECD   ";
char str[100]="";
char receiveData[100]="";
char password[30]="";
u8 rx_flg=0;
u32 adc_time=0;
double vol1=0;
double vol2=0;
char mima;
char zidian[26]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','X','Y','Z','@','#','$','%','&','=','+'};
u32 ccr=0;
double fre=0;
double fre_fin=0;
double out_fre=10;
double duty=20;
u32 out_time=5;
u8 channel_flg=0;
double ARR=100;
u8 password_flg=0;
char set_password[30]="A35#";
u32 out_flg=0;
u32 pwm_time=0;
u32 rest_time=0;
u32 success_count=0;
double last_s_fre=0;
double last_f_fre=0;
u32 fail_count=0;
u32 mima_wei=0;
RTC_TimeTypeDef H_M_S_last_s_Time;
RTC_TimeTypeDef H_M_S_last_f_Time;
u8 choose_flg=0;
u8 chance_count=3;
u32 fail_time=0;
u8 first_s_flg=0;
u8 first_f_flg=0;
char uart_set_password[30]="";
u8 led1=0;
u8 led2=0;
u8 led3=0;
u8 led_total=0;
u8 blink_flg=0;
u8 blink_mask=0;
u32 blink_time=0;
u8 lock_flg=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void key_pro(){
	if(uwTick-key_time<10)return;
	key_time=uwTick;
	key_state=key_scanf();
	if(key_state==1)key_value=1;
	if(key_state==2)key_value=2;
	if(key_state==3)key_value=3;
	if(key_state==4)key_value=4;
	if(key_state==5)key_value=5;
	if(key_state==6)key_value=6;
	if(key_value==1){
		if(lcd_state!=3){	
			LCD_Clear(Black);
			lcd_state++;
			if(lcd_state>2)lcd_state=0;
		}
		key_value=0;
	}
	if(key_value==2){
		if(lcd_state==0){
			password[mima_wei]=mima;
			mima_wei++;
		}else if(lcd_state==1){
			choose_flg+=1;
			if(choose_flg>3)choose_flg=0;
		}
		key_value=0;
	}
	if(key_value==3){
		if(lcd_state==0){
			fre_fin=(fre+vol2)/1000;
			if(!strcmp(password,set_password)){
				lcd_state=3;
				LCD_Clear(Black);
				out_flg=1;
				pwm_time=uwTick;
				password_flg=0;
				chance_count=3;
				password_flg=0;
				rest_time=out_time;
				first_s_flg=1;
				H_M_S_last_s_Time.Hours=H_M_S_Time.Hours;
				H_M_S_last_s_Time.Minutes=H_M_S_Time.Minutes;
				H_M_S_last_s_Time.Seconds=H_M_S_Time.Seconds;
				success_count++;
				last_s_fre=fre_fin;
				sprintf(str,"S:%02d%02d%02d%02d%02d%02d",Y_M_D_Time.Year,Y_M_D_Time.Month,Y_M_D_Time.Date,H_M_S_Time.Hours,H_M_S_Time.Minutes,H_M_S_Time.Seconds);
				HAL_UART_Transmit(&huart1,(u8*)str,sizeof(str),0xffff);
			}else{
				lcd_state=3;
				LCD_Clear(Black);
				chance_count--;
				fail_time=uwTick;
				if(chance_count==0)password_flg=2;
				else password_flg=1;
				first_f_flg=1;
				H_M_S_last_f_Time.Hours=H_M_S_Time.Hours;
				H_M_S_last_f_Time.Minutes=H_M_S_Time.Minutes;
				H_M_S_last_f_Time.Seconds=H_M_S_Time.Seconds;
				fail_count++;
				last_f_fre=fre_fin;
				sprintf(str,"F:%02d%02d%02d%02d%02d%02d",Y_M_D_Time.Year,Y_M_D_Time.Month,Y_M_D_Time.Date,H_M_S_Time.Hours,H_M_S_Time.Minutes,H_M_S_Time.Seconds);
				HAL_UART_Transmit(&huart1,(u8*)str,sizeof(str),0xffff);
			}
		}else if(lcd_state==1){
			switch(choose_flg){
				case 0:
					out_fre++;
				break;
				case 1:
					duty+=5;
				break;
				case 2:
					out_time++;
				break;
				case 3:
					channel_flg^=1;
				break;
			}
		}
		key_value=0;
	}
	if(key_value==4){
		if(lcd_state==1){
			switch(choose_flg){
				case 0:
					out_fre--;
				break;
				case 1:
					duty-=5;
				break;
				case 2:
					out_time--;
				break;
				case 3:
					channel_flg^=1;
				break;
			}
		}
		key_value=0;
	}
	if(key_value==5){
		if(lcd_state==0){
			memset(password,0,sizeof(password));
			mima_wei=0;
		}
		if(lcd_state==2){
			if(qingkong_flg==1){
				first_f_flg=0;
				first_s_flg=0;
				fail_count=0;
				success_count=0;
			}
		}
		key_value=0;
	}
	
}
void lcd_pro(){
	if(uwTick-lcd_time<100)return;
	lcd_time=uwTick;
	HAL_RTC_GetTime(&hrtc,&H_M_S_Time,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc,&Y_M_D_Time,RTC_FORMAT_BIN);
	if(lcd_state==0){
		
		LCD_DisplayStringLine(Line1,(u8*)title1);
		sprintf(str,"     UN:%s                   ",password);
		LCD_DisplayStringLine(Line3,(u8*)str);
		sprintf(str,"     %02d-%02d-%02d      ",Y_M_D_Time.Year,Y_M_D_Time.Month,Y_M_D_Time.Date);
		LCD_DisplayStringLine(Line5,(u8*)str);
		sprintf(str,"     %02d:%02d:%02d       ",H_M_S_Time.Hours,H_M_S_Time.Minutes,H_M_S_Time.Seconds);
		LCD_DisplayStringLine(Line6,(u8*)str);
		sprintf(str,"     PASS:%c  ",mima);
		LCD_DisplayStringLine(Line7,(u8*)str);
	}else if(lcd_state==1){
		
			LCD_DisplayStringLine(Line1,(u8*)title2);
			sprintf(str,"     F:%.1fKHz  ",out_fre);
			LCD_DisplayStringLine(Line3,(u8*)str);
			sprintf(str,"     D:%.0f%%  ",duty);
			LCD_DisplayStringLine(Line4,(u8*)str);
			sprintf(str,"     T:%d  ",out_time);
			LCD_DisplayStringLine(Line5,(u8*)str);
			if(channel_flg==0)
			sprintf(str,"     CH:PA6  ");
			else sprintf(str,"     CH:PA7  ");
			LCD_DisplayStringLine(Line6,(u8*)str);
			
		
	}else if(lcd_state==2){
		LCD_DisplayStringLine(Line1,(u8*)title4);
		if(first_s_flg==0){
			sprintf(str,"  SN=NA                ");
			LCD_DisplayStringLine(Line3,(u8*)str);
			sprintf(str,"  ST=NA                ");
			LCD_DisplayStringLine(Line4,(u8*)str);
		}else {
			sprintf(str,"  SN=%d     ",success_count);
			LCD_DisplayStringLine(Line3,(u8*)str);
			sprintf(str,"  ST=%.1f@%d:%d:%d     ",last_s_fre,H_M_S_last_s_Time.Hours,H_M_S_last_s_Time.Minutes,H_M_S_last_s_Time.Seconds);
			LCD_DisplayStringLine(Line4,(u8*)str);
		}
		if(first_f_flg==0){ 
			sprintf(str,"  FN=NA                      ");
			LCD_DisplayStringLine(Line5,(u8*)str);
			sprintf(str,"  FT=NA                       ");
			LCD_DisplayStringLine(Line6,(u8*)str);
		}else {
			sprintf(str,"  FN=%d     ",fail_count);
			LCD_DisplayStringLine(Line5,(u8*)str);
			sprintf(str,"  FT=%.1f@%d:%d:%d     ",last_f_fre,H_M_S_last_f_Time.Hours,H_M_S_last_f_Time.Minutes,H_M_S_last_f_Time.Seconds);
			LCD_DisplayStringLine(Line6,(u8*)str);
		}
	}else if(lcd_state==3){
		LCD_DisplayStringLine(Line1,(u8*)title3);
		if(password_flg==0){		
			sprintf(str,"     T:%02d   ",rest_time);
			LCD_DisplayStringLine(Line3,(u8*)str);
			sprintf(str,"     OF:%.1fKHz   ",out_fre);
			LCD_DisplayStringLine(Line4,(u8*)str);
			sprintf(str,"     OD:%.0f%%   ",duty);
			LCD_DisplayStringLine(Line5,(u8*)str);
			fre_fin=(fre+vol2)/1000;
			sprintf(str,"     CF:%.1fKHz   ",fre_fin);
			LCD_DisplayStringLine(Line6,(u8*)str);
		}else if(password_flg==1){
			
			sprintf(str,"       FAIL       ");
			LCD_DisplayStringLine(Line3,(u8*)str);
			sprintf(str,"      RETRY:%d          ",chance_count);
			LCD_DisplayStringLine(Line4,(u8*)str);
			if(uwTick-fail_time>3000){
				lcd_state=0;
				LCD_Clear(Black);
				memset(password,0,sizeof(password));
				mima_wei=0;
			}
		}else {
			lock_flg=1;
			sprintf(str,"       LOCK       ");
			LCD_DisplayStringLine(Line3,(u8*)str);
			sprintf(str,"      RETRY:%d          ",chance_count);
			LCD_DisplayStringLine(Line4,(u8*)str);
			sprintf(str,"      WAIT:%02d          ",10-((uwTick-fail_time)/1000));
			LCD_DisplayStringLine(Line5,(u8*)str);
			if(uwTick-fail_time>10000){
				lcd_state=0;
				LCD_Clear(Black);
				chance_count=3;
				memset(password,0,sizeof(password));
				mima_wei=0;
				lock_flg=0;
			}
		}
	}
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	if(huart==&huart1){
		receiveData[Size]='\0';
		rx_flg=1;
		HAL_UARTEx_ReceiveToIdle_IT(&huart1,(u8*)receiveData,sizeof(receiveData));
	}
}
void rx_pro(){
	if(rx_flg==1){
		int tmp_hour, tmp_min, tmp_sec;
		int tmp_year, tmp_month, tmp_date;
		if(sscanf(receiveData,"TIME:%02d%02d%02d",&H_M_S_setTime.Hours,&H_M_S_setTime.Minutes,&H_M_S_setTime.Seconds)==3){
			HAL_RTC_SetTime(&hrtc,&H_M_S_setTime,RTC_FORMAT_BIN);
			HAL_UART_Transmit(&huart1,(u8*)"OK",strlen("OK"),0xffff);
		}else if(!strcmp(receiveData,"?")){
			HAL_UART_Transmit(&huart1,(u8*)set_password,strlen(set_password),0xffff);
		}else if(sscanf(receiveData,"SET:%s",uart_set_password)==1){
			if(strlen(uart_set_password)<=10){
				strcpy(set_password,uart_set_password);
				HAL_UART_Transmit(&huart1,(u8*)"OK",strlen("OK"),0xffff);
			}else HAL_UART_Transmit(&huart1,(u8*)"ERROR",strlen("ERROR"),0xffff);
		}else if(sscanf(receiveData,"DATE:%02d%02d%02d", &tmp_year, &tmp_month, &tmp_date)==3){
            // 【修改点 3】：日期提取成功后，再赋值给 8 位的 RTC 结构体
            Y_M_D_setTime.Year = tmp_year;
            Y_M_D_setTime.Month = tmp_month;
            Y_M_D_setTime.Date = tmp_date;
            
			HAL_RTC_SetDate(&hrtc,&Y_M_D_setTime,RTC_FORMAT_BIN);
			HAL_UART_Transmit(&huart1,(u8*)"OK",strlen("OK"),0xffff);
            
		}else if(sscanf(receiveData,"PA6:%lf:%d%%:%d",&uart_fre,&uart_duty,&uart_time)==3){
				out_fre=uart_fre/1000;
				duty=uart_duty;
				rest_time=uart_time;
				channel_flg=0;
				out_flg=1;
				pwm_time=uwTick;
			}else if(sscanf(receiveData,"PA7:%lf:%d%%:%d",&uart_fre,&uart_duty,&uart_time)==3){
				out_fre=uart_fre/1000;
				duty=uart_duty;
				rest_time=uart_time;
				channel_flg=1;
				out_flg=1;
				pwm_time=uwTick;
			}
		else{
			HAL_UART_Transmit(&huart1,(u8*)"ERROR",strlen("ERROR"),0xffff);
		}
	rx_flg=0;
	}
	
}
void adc_pro(){
	u32 vol=0;
	if(uwTick-adc_time<100)return;
	adc_time=uwTick;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_Start(&hadc2);
	HAL_ADC_PollForConversion(&hadc1,1000);
	HAL_ADC_PollForConversion(&hadc2,1000);
	vol2=(HAL_ADC_GetValue(&hadc2)*3.3/4096)*(500/1.65)-500;
	vol1=HAL_ADC_GetValue(&hadc1)*255/4096;
	vol=vol1/10;
	mima=zidian[vol];
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim){
	if(htim==&htim3){
		if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_1){
			ccr=HAL_TIM_ReadCapturedValue(&htim3,TIM_CHANNEL_1);
			TIM3->CNT=0;
			fre=1000000/ccr;
		}
	}
}
void pwm_pro(){
	ARR=1000000/(out_fre*1000);
	
	if(out_flg==0){
		__HAL_TIM_SetCompare(&htim17,TIM_CHANNEL_1,0);
		__HAL_TIM_SetCompare(&htim16,TIM_CHANNEL_1,0);
	}else{
		if(channel_flg==0){
			__HAL_TIM_SetCompare(&htim17,TIM_CHANNEL_1,0);
			__HAL_TIM_SetAutoreload(&htim16,ARR);
			__HAL_TIM_SetCompare(&htim16,TIM_CHANNEL_1,duty/100*ARR);
		}else{
			__HAL_TIM_SetCompare(&htim16,TIM_CHANNEL_1,0);
			__HAL_TIM_SetAutoreload(&htim17,ARR);
			__HAL_TIM_SetCompare(&htim17,TIM_CHANNEL_1,duty/100*ARR);
		}
		if(uwTick-pwm_time>1000){
			rest_time-=1;
			pwm_time=uwTick;
		}
		if(rest_time==0){
			out_flg=0;
			if(lcd_state==3){
				lcd_state=0;
				LCD_Clear(Black);
				memset(password,0,sizeof(password));
				mima_wei=0;
			}
		}
	}
}
void led_disp(u8 led_num){
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC,led_num<<8,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}
void led_blink(){
	if(uwTick-blink_time<200)return;
	blink_time=uwTick;
	blink_flg^=1;
}
void led_app(){
	if(out_flg==1)led1=0x01;
	else led1=0;
	if(lock_flg==1)led2=0x02;
	else led2=0;
	if(lcd_state==0&&!HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)){
		led_blink();
		led3=blink_flg? 0:0x04;
	}else led3=0;
	led_total=led1|led2|led3;
	led_disp(led_total);
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
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	LCD_Init();
	LCD_Clear(Black);
	LCD_SetBackColor(Black);
	LCD_SetTextColor(White);
	HAL_UARTEx_ReceiveToIdle_IT(&huart1,(u8*)receiveData,sizeof(receiveData));
	HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2,ADC_SINGLE_ENDED);
	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim16,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim17,TIM_CHANNEL_1);
	led_disp(0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    key_pro();
	adc_pro();
	lcd_pro();
	 pwm_pro();
	rx_pro();
	  led_app();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
