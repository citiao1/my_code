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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
unsigned char Read_KEY(void);
void start(void);
void stop(void);
void shanshuo(unsigned int shanshuo_pinlv);
void LED_Init(void);
void LED_ON(unsigned short int Pin);
void LED_OFF(unsigned short int Pin);
void LED_To(unsigned short int Pin);
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
  /* USER CODE BEGIN 2 */
	LED_Init();
	uint8_t count=0;
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		if(Read_KEY()==1){
			count++;
		}
		
		if(count%2!=0){
			start();
		}
		else{
			stop();
		}
		
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void LED_Init(void){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, LD6_Pin|LD7_Pin|LD8_Pin|LD1_Pin|LD2_Pin|LD3_Pin|LD4_Pin|LD5_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, LD6_Pin|LD7_Pin|LD8_Pin|LD1_Pin|LD2_Pin|LD3_Pin|LD4_Pin|LD5_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPIOC, LD6_Pin|LD7_Pin|LD8_Pin|LD1_Pin|LD2_Pin|LD3_Pin|LD4_Pin|LD5_Pin, GPIO_PIN_SET);
}
void LED_ON(unsigned short int Pin){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_RESET);
}
void LED_OFF(unsigned short int Pin){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_RESET);
}
void LED_To(unsigned short int Pin){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOC, Pin);
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_RESET);
}
unsigned char Read_KEY(void){
	unsigned char value=0;
	if(HAL_GPIO_ReadPin(B1_GPIO_Port,B1_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B1_GPIO_Port,B1_Pin)==GPIO_PIN_RESET){
				value=1;
				while(HAL_GPIO_ReadPin(B1_GPIO_Port,B1_Pin)==GPIO_PIN_RESET);
			}
		}
	if(HAL_GPIO_ReadPin(B2_GPIO_Port,B2_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B2_GPIO_Port,B2_Pin)==GPIO_PIN_RESET){
				value=2;
				//while(HAL_GPIO_ReadPin(B2_GPIO_Port,B2_Pin)==GPIO_PIN_RESET);
			}
		}
	if(HAL_GPIO_ReadPin(B3_GPIO_Port,B3_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B3_GPIO_Port,B3_Pin)==GPIO_PIN_RESET){
				value=3;
				//while(HAL_GPIO_ReadPin(B3_GPIO_Port,B3_Pin)==GPIO_PIN_RESET);
			}
		}
	if(HAL_GPIO_ReadPin(B4_GPIO_Port,B4_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B4_GPIO_Port,B4_Pin)==GPIO_PIN_RESET){
				value=4;
				//while(HAL_GPIO_ReadPin(B4_GPIO_Port,B4_Pin)==GPIO_PIN_RESET);
			}
		}
	return value;
}
	uint8_t a=0;
	uint16_t count1=0;
	uint16_t temp=0;
	uint16_t liushui_pinlv=100;
	uint16_t shanshuo_pinlv=100;
void start(void){
	if(Read_KEY()==2){
		count1++;
		HAL_Delay(200);
		if(count1%2==0){
			a=0;
		}else{
			a=1;
		}
	}
	else if(Read_KEY()==3){
		temp=a;
		a=3;
	}
	else if(Read_KEY()==4){
		temp=a;
		a=4;
	}
	switch(a){
		case 0:
			LED_ON(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD4_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			shanshuo(shanshuo_pinlv);
			LED_OFF(LD4_Pin);
			break;
		case 1:
			LED_ON(LD4_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD4_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			break;
		case 3:
			liushui_pinlv+=100;
			a=temp;
			break;
		case 4:
			shanshuo_pinlv+=100;
			a=temp;
			break;
	}
}
void stop(void){
	LED_OFF(LD1_Pin);
	LED_OFF(LD2_Pin);
	LED_OFF(LD3_Pin);
	LED_OFF(LD4_Pin);
	LED_OFF(LD5_Pin);
	LED_OFF(LD6_Pin);
	LED_OFF(LD7_Pin);
	LED_OFF(LD8_Pin);
}
void shanshuo(unsigned int shanshuo_pinlv){
		LED_ON(LD5_Pin);
		LED_ON(LD6_Pin);
		LED_ON(LD7_Pin);
		LED_ON(LD8_Pin);
		HAL_Delay(shanshuo_pinlv);
		LED_OFF(LD5_Pin);
		LED_OFF(LD6_Pin);
		LED_OFF(LD7_Pin);
		LED_OFF(LD8_Pin);
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
