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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "deng.h"
#include "anjian.h"
#include "string.h"
#include "stm32g4xx_it.h"
#include <stdio.h>
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
	unsigned char uart_data[1];
	unsigned char flag[10]="LED1ON";
	unsigned char flag1[10]="hello";
	unsigned char flag2[10]="nohello";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Rx_Process(void)
{
     if(Rx_flg ==1)
      {
            
//           if(Rx_Buff[0] == 'b' && Rx_Buff[1] == 'l' && Rx_Buff[2] == 'i' && Rx_Buff[3] == 'n' 
//               && Rx_Buff[4] == 'k'  )
           if(!strcmp((char*)Rx_Buff, (char*)"blink\r\n"))
           {
                
                HAL_UART_Transmit(&huart1,Rx_Buff, sizeof(Rx_Buff), 100);
           
           }
}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
		//if(huart->Instance==USART1)
  if(huart == &huart1)
  {
//        gucLed = aRX;
      
        Rx_Buff[Rx_cnt++] = aRX;
    
        if(0x0a == aRX)
        {
            Rx_flg = 1;
        }
        
        HAL_UART_Receive_DMA(&huart1, &aRX, sizeof(aRX));
  }
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	if(huart == &huart1)
  {
		HAL_UART_Transmit_DMA(&huart1, Rx_Buff, sizeof(Rx_Buff));	
		if(!strcmp((char*)Rx_Buff, (char*)"blink\r\n"))
           {
                
                HAL_UART_Transmit(&huart1,Rx_Buff, sizeof(Rx_Buff), 100);
           
           }
			HAL_UART_Receive_DMA(&huart1, Rx_Buff, sizeof(Rx_Buff));
        
  }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
		
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

	LED_Init();
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1,Rx_Buff,sizeof(Rx_Buff));
	HAL_TIM_Base_Start(&htim4);	
	int counter=0;
	char message[20];
	//Uart_Tx(uart_data,1);
	LED_OFF(LD1_Pin);
	LED_OFF(LD2_Pin);
	LED_OFF(LD3_Pin);
	LED_OFF(LD4_Pin);
	LED_OFF(LD5_Pin);
	LED_OFF(LD6_Pin);
	LED_OFF(LD7_Pin);
	LED_OFF(LD8_Pin);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		counter=__HAL_TIM_GET_COUNTER(&htim4);
		sprintf(message,"counter:%d",counter);
		HAL_UART_Transmit_DMA(&huart1,(uint8_t*)message,strlen(message));
		HAL_Delay(99);
		if(key_value==2){
				LED_ON(LD1_Pin);
				HAL_Delay(500);
				LED_OFF(LD1_Pin);
				LED_ON(LD2_Pin);
				HAL_Delay(500);
				LED_OFF(LD2_Pin);
				LED_ON(LD3_Pin);
				HAL_Delay(500);
				LED_OFF(LD3_Pin);
				LED_ON(LD4_Pin);
				HAL_Delay(500);
				LED_OFF(LD4_Pin);
		}else if(key_value==3){
				LED_ON(LD4_Pin);
				HAL_Delay(500);
				LED_OFF(LD4_Pin);
				LED_ON(LD3_Pin);
				HAL_Delay(500);
				LED_OFF(LD3_Pin);
				LED_ON(LD2_Pin);
				HAL_Delay(500);
				LED_OFF(LD2_Pin);
				LED_ON(LD1_Pin);
				HAL_Delay(500);
				LED_OFF(LD1_Pin);
		}else if(count%2){
				LED_ON(LD5_Pin);
				LED_ON(LD6_Pin);
				LED_ON(LD7_Pin);
				LED_ON(LD8_Pin);
				HAL_Delay(500);
				LED_OFF(LD5_Pin);
				LED_OFF(LD6_Pin);
				LED_OFF(LD7_Pin);
				LED_OFF(LD8_Pin);
				HAL_Delay(500);
		}	else if(!count%2){
				LED_OFF(LD5_Pin);
				LED_OFF(LD6_Pin);
				LED_OFF(LD7_Pin);
				LED_OFF(LD8_Pin);
		}
		
		/*switch (uart_data[0]){
			case 1:
				LED_ON(LD1_Pin);
				Uart_Tx(flag,10);
				HAL_Delay(1000);
				LED_OFF(LD1_Pin);
				uart_data[0]=0;
				break;
			case 2:
				LED_ON(LD2_Pin);
				HAL_Delay(1000);
				LED_OFF(LD2_Pin);
				uart_data[0]=0;
			
				break;
			case 3:
				LED_ON(LD3_Pin);
				HAL_Delay(1000);
				LED_OFF(LD3_Pin);
				uart_data[0]=0;
				break;
			case 4:
				LED_ON(LD4_Pin);
				HAL_Delay(1000);
				LED_OFF(LD4_Pin);
				uart_data[0]=0;
				break;
			case 5:
				LED_ON(LD5_Pin);
				HAL_Delay(1000);
				LED_OFF(LD5_Pin);
				uart_data[0]=0;
				break;
			case 6:
				LED_ON(LD6_Pin);
				HAL_Delay(1000);
				LED_OFF(LD6_Pin);
				uart_data[0]=0;
				break;
			case 7:
				LED_ON(LD7_Pin);
				HAL_Delay(1000);
				LED_OFF(LD7_Pin);
				uart_data[0]=0;
				break;
			case 8:
				LED_ON(LD8_Pin);
				HAL_Delay(1000);
				LED_OFF(LD8_Pin);
				uart_data[0]=0;
				break;
			default:
				break;
		}
		*/




	}
		
	}
			
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


  /* USER CODE END 3 */

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
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 12;
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
