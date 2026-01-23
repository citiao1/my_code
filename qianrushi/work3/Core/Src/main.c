/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//uint8_t Rx,Tx = 0;
#define B1_LED 0x01  
#define B2_LED 0x02  
#define B3_LED 0x04  
#define B4_LED 0x08 
uint8_t gucLed = 0;

u8 KeyValue;        //0-初始， 1-闪烁， 2-流水
u8 flow_direction = 0; 
u32 ShanStick;
u32 FlowStick;
u32 uwTickUart;
u32 keyTime;
u32 uwTick_LED_Speed_nms;
u8 blink_flag = 0;
u8 high_blink_flag = 0;
static uint8_t led_state = 0x00;
u32 uwTick_Blink;
uint8_t key_led_state = 0x00; 
uint8_t key_led_state1 = 0x00; 
char HelloStr[] = "Hello world\r\n";
char Ackon[] = "LED on ok\r\n";
char Ackoff[] = "LED off ok\r\n";

char Ackerr[] = "Error\r\n";

char Ackblink[] = "blink ok\r\n";
char Ackflow[]  = "flow ok\r\n";

char flowleft[] = "f left\r\n";
char Ackflowleft[] ="flow left ok\r\n";

char flowRht[] = "f right\r\n";
char AckRhtleft[] ="flow Right ok\r\n";


static uint8_t gLedBitL;
static uint8_t gLedBitR;

int gTickBlink;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t enable_flow = 0;      
uint8_t enable_blink = 0; 
uint32_t blink_timer = 0;      
uint8_t blink_state = 0;       
uint8_t blink_mask = 0xF0;
uint32_t flow_interval = 500;  
uint32_t blink_interval = 200;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Rx_Process(void);

void LED_Disp(volatile uint8_t Led_num);
void LED_DispSingle(uint8_t Led_num);

uint8_t FlowLedLeft_app(volatile uint8_t Led_Bit);
uint8_t FlowLedRh_app(volatile uint8_t Led_Bit);

void ALL0FF_LED();
void LED_Shine(uint8_t Led_num);
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
gLedBitR = LEDFLOWRB;
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1,&aRX,1);
  
//  for(u8 i=0;i<20;i++)
//  {
//        Rx_Buff[i] = 0;
//  }
//  gLedBitL = LEDFLOWLB; 
//  gLedBitR = LEDFLOWRB; 
//  Rx_flg =0;
//  Rx_cnt =0;
//  KeyValue  = 0;
  //gAppState = 1;
  HAL_Delay(200);
  memset(Rx_Buff, 0x00, sizeof(Rx_Buff));    
  ALL0FF_LED();
	uwTick_Blink = uwTick;
  HAL_UART_Transmit(&huart1,(u8 *) HelloStr, strlen(HelloStr), 0xffff);

//  Led_Disp(0X00);
//  gTickBlink = 500;
//  Rx = USART1->RDR; 
//  __HAL_UART_CLEAR_NEFLAG(&huart1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  

  
  while (1)
  {
      
 
			
      HAL_Delay(10);
      Rx_Process();
			uint8_t flow_led = 0;
			uint8_t blink_led = 0;
			if(!((uwTick - keyTime)<10))   {
				keyTime = uwTick; 
				KeyValue= key_scanf();
				}
      if(KeyValue==1){
				key_led_state=1;
			}else if(KeyValue==2){
				key_led_state=2;
			}else if(KeyValue==3){
				key_led_state1=3;
			}else if(KeyValue==4){
				key_led_state1=4;
			}

			switch(key_led_state){
				 case 1:
					  gLedBitL = FlowLedLeft_app(gLedBitL);
            flow_led = gLedBitL;
					break;
				 case 2:
						gLedBitR = FlowLedRh_app(gLedBitR);
            flow_led = gLedBitR;
					 break;
				 
				 default:
					 break;
			 
			 }
			switch(key_led_state1){
				case 3:
					 App_Low4Blink(blink_interval);
						blink_led = blink_state ? blink_mask : 0;
					 break;
				 case 4:
						blink_led=0;
					 break;
				 default:
					 break;
			
			}
			
			if (enable_flow) {
        if (flow_direction == 1) {
            gLedBitL = FlowLedLeft_app(gLedBitL);
            flow_led = gLedBitL;
        } else if (flow_direction == 2) {
            gLedBitR = FlowLedRh_app(gLedBitR);
            flow_led = gLedBitR;
        }
			}
      
				if (enable_blink) {
						App_Low4Blink(blink_interval); 
						blink_led = blink_state ? blink_mask : 0; 
				}

				
				uint8_t total_led = flow_led | blink_led ;
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

void Rx_Process(void)
{
     if(Rx_flg ==1)
      {
            
        if (!strcmp((char*)Rx_Buff, "left\r\n")) {
            enable_flow = 1;
            flow_direction = 1;
            HAL_UART_Transmit(&huart1, (u8*)Ackflowleft, strlen(Ackflowleft), 0xffff);
        }
				else if (!strcmp((char*)Rx_Buff, "meanwhile blink flow\r\n")) {
                enable_flow = 1;
								flow_direction = 1;
								enable_blink = 1;
                HAL_UART_Transmit(&huart1, (u8*)"meanwhile ok\r\n", strlen("meanwhile ok\r\n"), 0xffff);
            } 
       
        else if (!strcmp((char*)Rx_Buff, "right\r\n")) {
            enable_flow = 1;
            flow_direction = 2;
            HAL_UART_Transmit(&huart1, (u8*)AckRhtleft, strlen(AckRhtleft), 0xffff);
        }
        
        else if (!strcmp((char*)Rx_Buff, "stop_flow\r\n")) {
            enable_flow = 0;
            HAL_UART_Transmit(&huart1, (u8*)"flow stopped\r\n", 13, 0xffff);
        }
       
        
        
        else if (!strcmp((char*)Rx_Buff, "stop_blink\r\n")) {
            enable_blink = 0;
            blink_state = 0; 
            HAL_UART_Transmit(&huart1, (u8*)"blink stopped\r\n", 15, 0xffff);
        }
				else if (strstr((char*)Rx_Buff, "blink") != NULL) {
            uint32_t new_freq;
            if (sscanf((char*)Rx_Buff, "blink %u", &new_freq) == 1) {
                
                
                    blink_interval = new_freq;  
                    enable_blink = 1;           
                    
                    
                    HAL_UART_Transmit(&huart1, (u8*)"Order ok\r\n", strlen("Order ok\r\n"), 0xffff);
						}
            
            else if (!strcmp((char*)Rx_Buff, "blink\r\n")) {
                enable_blink = 1;
                HAL_UART_Transmit(&huart1, (u8*)"blink started\r\n", strlen("blink started\r\n"), 0xffff);
            } 
           
            
        }else if (strstr((char*)Rx_Buff, "flow") != NULL) {
            uint32_t new_freq;
            if (sscanf((char*)Rx_Buff, "flow %u", &new_freq) == 1) {
                
                
                    flow_interval = new_freq;  
                    enable_flow = 1;           
                    
                    HAL_UART_Transmit(&huart1, (u8*)"Order ok\r\n", strlen("Order ok\r\n"), 0xffff);
						}
					}else if (!strcmp((char*)Rx_Buff, "OFF\r\n")) {
                
								
								key_led_state=0;
								key_led_state1=0;
								enable_blink = 0;
								blink_state = 0; 
								enable_flow = 0;
                HAL_UART_Transmit(&huart1, (u8*)"OFF\r\n", strlen("OFF\r\n"), 0xffff);
            } 

           //Error
           else{
               HAL_UART_Transmit(&huart1,(u8 *) Ackerr, strlen(Ackerr), 0xffff);
					 }
          //Clear Rx_Buff
            memset(Rx_Buff,0x00, 256);
            Rx_cnt =0;
            Rx_flg =0;
      
      
}

}
/*
APP任务周期：循环时间500ms
APP任务名：FlowLedLeft_app；流水灯左移
入口：LedBit-开始位。      宏LEDFLOWLE-结束位，  方向<<L  
出口：当前流动的位Led_Bit
功能：控制几位LED流水灯，控制方向，向左，
       
*/
uint8_t FlowLedLeft_app(volatile uint8_t Led_Bit)
{
    
    //Led_Bit = Led_Bit;
    if((uwTick - uwTick_LED_Speed_nms)<flow_interval)   {return Led_Bit;}
        uwTick_LED_Speed_nms = uwTick; 
    
    LED_Disp(Led_Bit);  
    //LED_DispSingle(0x80);    //点亮一个led，0x80-LD8  0x01-LD1,0x00-熄灭。函数可以加一个参数Single_Bit
    //HAL_Delay(200);
    Led_Bit = Led_Bit<<1;  
    if(Led_Bit == LEDFLOWLE)   Led_Bit =LEDFLOWLB;
    return  Led_Bit;

}
/*
APP任务周期：循环时间500ms
APP任务名：流水灯右移
入口：LedBit-开始位。      宏LEDFLOWRE-结束位，  方向 >>R
出口：当前流动的位Led_Bit
功能：控制几位LED流水灯，控制方向，向右
       app任务循环时间100ms
*/
uint8_t FlowLedRh_app(volatile uint8_t Led_Bit)
{
    
    //Led_Bit = Led_Bit;
    if((uwTick - uwTick_LED_Speed_nms)<flow_interval)   {return Led_Bit;}
    uwTick_LED_Speed_nms = uwTick; 
    
    LED_Disp(Led_Bit); 
    //LED_DispSingle(0x01); 	//点亮一个led，0x80-LD8  0x01-LD1,0x00-熄灭。函数可以加一个参数Single_Bit
    //HAL_Delay(300);
    Led_Bit = Led_Bit>>1;  
    if(Led_Bit == LEDFLOWRE)   Led_Bit =LEDFLOWRB;
    return  Led_Bit;

}
//任务周期：500ms
//任务名：
//功能：低4个灯闪烁
//入口：无
//出口：无
void App_Low4Blink(int interval)
{
    static  u8 BlinkBit = 0xF0;
    if (uwTick - blink_timer < interval) {
        return; // ??????,????
    }
    blink_timer = uwTick;       // ?????
    blink_state ^= 1;
    

}

//函数名：LED_Disp
//入口：uint8_t Led_num
//出口：void
//功能：先熄灭所有灯，led_num 的D0—D7对应LD1-LD8点亮,0x01-LD1;0x02-LD2;..   0x10-LD5;0x20--LD6...

void LED_Disp(volatile uint8_t Led_num)
{
    /*熄灭全部LED 必须*/
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12,GPIO_PIN_SET);
    UNLOCKLED;
    LOCKLED;
    //点亮某几个LED	
    HAL_GPIO_WritePin(GPIOC,Led_num<<8,GPIO_PIN_RESET);
    UNLOCKLED;
    LOCKLED;
}

//函数名：LED_DispSingle
//入口：uint8_t Led_num
//出口：void
//功能：只点亮，led_num 的D0—D7对应LD1-LD8点亮,0x01-LD1;0x02-LD2;..   0x10-LD5;0x20--LD6...

void LED_DispSingle(uint8_t Led_num)
{
   
    //点亮某LED	
    HAL_GPIO_WritePin(GPIOC,Led_num<<8,GPIO_PIN_RESET);
    UNLOCKLED;
    LOCKLED;
}
//入口：位数1-LD1  2—LD2,4-LD3
//出口
//功能：翻转某位，灯闪烁
void LED_Shine(uint8_t Led_num)
{
   
    //点亮某LED	
    HAL_GPIO_TogglePin(GPIOC,Led_num<<8);
    UNLOCKLED;
    LOCKLED;
}

//熄灭所有灯
void ALL0FF_LED()
{
    /*熄灭全部LED*/
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12,GPIO_PIN_SET);
    UNLOCKLED;
    LOCKLED;
    //点亮某LED	
//    HAL_GPIO_WritePin(GPIOC,Led_num<<8,GPIO_PIN_RESET);
//    UNLOCKLED;
//    LOCKLED;
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

#ifdef  USE_FULL_ASSERT
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
