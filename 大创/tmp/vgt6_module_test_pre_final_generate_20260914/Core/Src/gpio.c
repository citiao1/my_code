/* USER CODE BEGIN Header */
/**
  * @file    gpio.c
  * @brief   GPIO initialization generated for the VGT6 module test.
  */
/* USER CODE END Header */
#include "gpio.h"

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOD, OLED_DC_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, OLED_RST_Pin | OLED_DIN_Pin | OLED_CLK_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = IMU_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(IMU_CS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = IMU_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(IMU_INT1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = OLED_DC_Pin | OLED_RST_Pin | OLED_DIN_Pin | OLED_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = STATUS_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(STATUS_LED_GPIO_Port, &GPIO_InitStruct);
}
