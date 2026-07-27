/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

#include "Emm_V5.h"
#include "font.h"
#include "oled.h"

/* 只有校准零点时才临时改成1，校准并烧录后必须恢复为0。 */
#define CALIBRATE_MOTOR_1_ON_BOOT  0
#define CALIBRATE_MOTOR_2_ON_BOOT  0

#define MOTOR_1_ADDRESS            1U
#define MOTOR_2_ADDRESS            2U
#define MOTOR_BROADCAST_ADDRESS    0U
#define MOTOR_STEP_PULSES          267UL
#define MOTOR_SPEED_RPM            180U
#define MOTOR_ACCELERATION         40U
#define MOTOR_RELATIVE_MODE        0U
#define MOTOR_HOME_MODE            0U

#define COMMAND_GAP_MS             100UL
#define HOME_WAIT_MS               3000UL
#define POSITION_POLL_MS           150UL
#define POSITION_TIMEOUT_MS        600UL
#define BUTTON_DEBOUNCE_MS         30UL
#define DISPLAY_REFRESH_MS         200UL

static const uint8_t motor_address[2] =
{
  MOTOR_1_ADDRESS, MOTOR_2_ADDRESS
};

static float motor_angle[2];
static uint8_t position_valid[2];
static uint8_t poll_motor;
static uint8_t requested_motor;
static uint8_t waiting_position;
static uint32_t request_started_ms;
static uint32_t last_poll_ms;
static uint32_t last_display_ms;
static uint32_t key_count;
static uint32_t command_count;
static uint32_t receive_error_count;

static uint8_t key_raw;
static uint8_t key_stable;
static uint32_t key_changed_ms;

/* 使用可修改字符数组，避免ARMCC把格式字符串放入.conststring。 */
static char line_motor_1[] = {'M','1',' ','A','N','G',' ','-','-','-','-','-','-','\0'};
static char line_motor_2[] = {'M','2',' ','A','N','G',' ','-','-','-','-','-','-','\0'};
static char line_key[]     = {'K','E','Y',' ','0','0','0','\0'};
static char line_command[] = {'C','M','D',' ','0','0','0','\0'};
static char line_receive[] = {'R','X',' ','0',' ','V','1',' ','0',' ','V','2',' ','0','\0'};
static char line_error[]   = {'E','R','R',' ','0','0','0','\0'};
static char line_mode[]    = {'R','E','L',' ','P','R','E','V',' ','T','A','R','G','E','T','\0'};
static char line_step[]    = {'C','W',' ','3','0',' ','D','E','G','\0'};
static char line_homing[]  = {'H','O','M','I','N','G',' ','M','1',' ','M','2','\0'};

void SystemClock_Config(void);

static void PutNumber(char *text, uint8_t position, uint32_t value,
                      uint8_t digits)
{
  uint32_t divisor = 1UL;
  uint8_t index;

  for (index = 1U; index < digits; ++index)
  {
    divisor *= 10UL;
  }
  for (index = 0U; index < digits; ++index)
  {
    text[position + index] = (char)('0' + (value / divisor) % 10UL);
    divisor /= 10UL;
  }
}

static void PutAngle(char *text, uint8_t position, float angle, uint8_t valid)
{
  int32_t tenths;
  uint32_t magnitude;
  uint8_t index;

  if (valid == 0U)
  {
    for (index = 0U; index < 6U; ++index)
    {
      text[position + index] = '-';
    }
    return;
  }

  tenths = (int32_t)(angle * 10.0f + ((angle < 0.0f) ? -0.5f : 0.5f));
  magnitude = (uint32_t)((tenths < 0) ? -tenths : tenths);
  text[position] = (tenths < 0) ? '-' : '+';
  PutNumber(text, position + 1U, (magnitude / 10UL) % 1000UL, 3U);
  text[position + 4U] = '.';
  PutNumber(text, position + 5U, magnitude % 10UL, 1U);
}

int main(void)
{
  uint32_t now_ms;

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();

  /* OLED直接使用移植后的SSD1306库，没有中间显示封装。 */
  OLED_Init();
  OLED_NewFrame();
  OLED_PrintASCIIString(0U, 24U, line_homing, &afont8x6,
                        OLED_COLOR_NORMAL);
  OLED_ShowFrame();

  /* 上电后依次使能两台电机。 */
  HAL_Delay(500U);
  Emm_V5_En_Control(MOTOR_1_ADDRESS, true, false);
  HAL_Delay(COMMAND_GAP_MS);
  Emm_V5_En_Control(MOTOR_2_ADDRESS, true, false);
  HAL_Delay(COMMAND_GAP_MS);

#if CALIBRATE_MOTOR_1_ON_BOOT
  Emm_V5_Origin_Set_O(MOTOR_1_ADDRESS, true);
  HAL_Delay(COMMAND_GAP_MS);
#endif
#if CALIBRATE_MOTOR_2_ON_BOOT
  Emm_V5_Origin_Set_O(MOTOR_2_ADDRESS, true);
  HAL_Delay(COMMAND_GAP_MS);
#endif

  /* RESET后两个地址都会收到单圈就近回零命令。 */
  Emm_V5_Origin_Trigger_Return(MOTOR_1_ADDRESS, MOTOR_HOME_MODE, false);
  HAL_Delay(COMMAND_GAP_MS);
  Emm_V5_Origin_Trigger_Return(MOTOR_2_ADDRESS, MOTOR_HOME_MODE, false);
  HAL_Delay(HOME_WAIT_MS);

  now_ms = HAL_GetTick();
  last_poll_ms = now_ms;
  last_display_ms = now_ms - DISPLAY_REFRESH_MS;
  key_raw = (HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) ==
             GPIO_PIN_RESET);
  key_stable = key_raw;
  key_changed_ms = now_ms;

  while (1)
  {
    uint8_t pressed;

    now_ms = HAL_GetTick();
    pressed = (HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) ==
               GPIO_PIN_RESET);

    /* PE0低电平有效，稳定按下30ms后只计数一次。 */
    if (pressed != key_raw)
    {
      key_raw = pressed;
      key_changed_ms = now_ms;
    }
    else if ((pressed != key_stable) &&
             ((uint32_t)(now_ms - key_changed_ms) >= BUTTON_DEBOUNCE_MS))
    {
      key_stable = pressed;
      if (key_stable != 0U)
      {
        ++key_count;

        /* 运动命令发送期间不接收查询回复，避免应答帧混在一起。 */
        (void)HAL_UART_AbortReceive(&huart2);
        waiting_position = 0U;
        uart2_position_received = 0U;
        __HAL_UART_CLEAR_OREFLAG(&huart2);

        /* raF=0：相对上一次输入目标位置继续顺时针增加30度。 */
        Emm_V5_Pos_Control(MOTOR_1_ADDRESS, 0U, MOTOR_SPEED_RPM,
                           MOTOR_ACCELERATION, MOTOR_STEP_PULSES,
                           MOTOR_RELATIVE_MODE, true);
        HAL_Delay(COMMAND_GAP_MS);
        Emm_V5_Pos_Control(MOTOR_2_ADDRESS, 0U, MOTOR_SPEED_RPM,
                           MOTOR_ACCELERATION, MOTOR_STEP_PULSES,
                           MOTOR_RELATIVE_MODE, true);
        HAL_Delay(COMMAND_GAP_MS);
        Emm_V5_Synchronous_motion(MOTOR_BROADCAST_ADDRESS);

        ++command_count;
        last_poll_ms = HAL_GetTick();
      }
    }

    /* DMA接收错误或超时后仅重新查询，不会锁住后续按键。 */
    if (uart2_receive_error != 0U)
    {
      uart2_receive_error = 0U;
      waiting_position = 0U;
      ++receive_error_count;
      (void)HAL_UART_AbortReceive(&huart2);
    }
    if ((waiting_position != 0U) &&
        ((uint32_t)(now_ms - request_started_ms) >= POSITION_TIMEOUT_MS))
    {
      waiting_position = 0U;
      ++receive_error_count;
      (void)HAL_UART_AbortReceive(&huart2);
    }

    /* 固定8字节位置回复收满后，HAL_UART_RxCpltCallback会置位。 */
    if (uart2_position_received != 0U)
    {
      uint8_t index = requested_motor;

      uart2_position_received = 0U;
      waiting_position = 0U;

      if ((uart2_position_frame[0] == motor_address[index]) &&
          (uart2_position_frame[1] == 0x36U) &&
          (uart2_position_frame[7] == 0x6BU))
      {
        uint32_t raw = ((uint32_t)uart2_position_frame[3] << 24) |
                       ((uint32_t)uart2_position_frame[4] << 16) |
                       ((uint32_t)uart2_position_frame[5] << 8) |
                       (uint32_t)uart2_position_frame[6];
        float angle = ((float)raw * 360.0f) / 65536.0f;

        if (uart2_position_frame[2] != 0U)
        {
          angle = -angle;
        }
        motor_angle[index] = angle;
        position_valid[index] = 1U;
        poll_motor = (uint8_t)((index + 1U) % 2U);
      }
      else
      {
        ++receive_error_count;
      }
    }

    /* 一次查询一个地址：先启动8字节DMA接收，再发送S_CPOS。 */
    if ((waiting_position == 0U) &&
        ((uint32_t)(now_ms - last_poll_ms) >= POSITION_POLL_MS))
    {
      (void)HAL_UART_AbortReceive(&huart2);
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      uart2_position_received = 0U;
      uart2_receive_error = 0U;

      if (HAL_UART_Receive_DMA(&huart2, uart2_position_frame,
                               UART2_POSITION_FRAME_SIZE) == HAL_OK)
      {
        requested_motor = poll_motor;
        Emm_V5_Read_Sys_Params(motor_address[requested_motor], S_CPOS);
        waiting_position = 1U;
        request_started_ms = now_ms;
      }
      else
      {
        ++receive_error_count;
      }
      last_poll_ms = now_ms;
    }

    /* 直接使用SSD1306库现有函数绘制并刷新整帧。 */
    if ((uint32_t)(now_ms - last_display_ms) >= DISPLAY_REFRESH_MS)
    {
      last_display_ms = now_ms;
      PutAngle(line_motor_1, 7U, motor_angle[0], position_valid[0]);
      PutAngle(line_motor_2, 7U, motor_angle[1], position_valid[1]);
      PutNumber(line_key, 4U, key_count % 1000UL, 3U);
      PutNumber(line_command, 4U, command_count % 1000UL, 3U);
      line_receive[3] = (waiting_position != 0U) ? '1' : '0';
      line_receive[8] = (position_valid[0] != 0U) ? '1' : '0';
      line_receive[13] = (position_valid[1] != 0U) ? '1' : '0';
      PutNumber(line_error, 4U, receive_error_count % 1000UL, 3U);

      OLED_NewFrame();
      OLED_PrintASCIIString(0U, 0U, line_motor_1, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 8U, line_motor_2, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 16U, line_key, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 24U, line_command, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 32U, line_receive, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 40U, line_error, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 48U, line_mode, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0U, 56U, line_step, &afont8x6,
                            OLED_COLOR_NORMAL);
      OLED_ShowFrame();
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
