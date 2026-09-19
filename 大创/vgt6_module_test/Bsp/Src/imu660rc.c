#include "imu660rc.h"

#include <string.h>

#include "main.h"
#include "spi.h"
#include "stm32f4xx_hal.h"

#define IMU660RC_REG_WHO_AM_I 0x0FU
#define IMU660RC_REG_CTRL1    0x10U
#define IMU660RC_REG_CTRL2    0x11U
#define IMU660RC_REG_CTRL3    0x12U
#define IMU660RC_REG_CTRL6    0x15U
#define IMU660RC_REG_CTRL8    0x17U
#define IMU660RC_REG_STATUS   0x1EU
#define IMU660RC_REG_OUT_G    0x22U
#define IMU660RC_SPI_TIMEOUT  10U
#define IMU660RC_MAX_PAYLOAD  16U

static Imu660rcDiagnostics imu_diagnostics;

static void Imu660rc_Select(void)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET);
}

static void Imu660rc_Deselect(void)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET);
}

static uint8_t Imu660rc_ReadRegisters(uint8_t reg, uint8_t *data,
                                      uint8_t length)
{
  uint8_t tx[IMU660RC_MAX_PAYLOAD + 1U] = {0};
  uint8_t rx[IMU660RC_MAX_PAYLOAD + 1U] = {0};
  HAL_StatusTypeDef status;

  if ((data == NULL) || (length == 0U) || (length > IMU660RC_MAX_PAYLOAD))
  {
    imu_diagnostics.transfer_ok = 0U;
    imu_diagnostics.hal_status = (uint8_t)HAL_ERROR;
    return 0U;
  }

  tx[0] = (uint8_t)(reg | 0x80U);
  Imu660rc_Select();
  status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, (uint16_t)length + 1U,
                                   IMU660RC_SPI_TIMEOUT);
  Imu660rc_Deselect();
  imu_diagnostics.hal_status = (uint8_t)status;
  if (status != HAL_OK)
  {
    imu_diagnostics.transfer_ok = 0U;
    return 0U;
  }

  memcpy(data, &rx[1], length);
  imu_diagnostics.transfer_ok = 1U;
  if (reg == IMU660RC_REG_WHO_AM_I)
  {
    imu_diagnostics.who_am_i = data[0];
  }
  else if (reg == IMU660RC_REG_STATUS)
  {
    imu_diagnostics.status_reg = data[0];
  }
  return 1U;
}

static uint8_t Imu660rc_WriteRegister(uint8_t reg, uint8_t value)
{
  uint8_t tx[2];
  HAL_StatusTypeDef status;

  tx[0] = (uint8_t)(reg & 0x7FU);
  tx[1] = value;
  Imu660rc_Select();
  status = HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), IMU660RC_SPI_TIMEOUT);
  Imu660rc_Deselect();
  imu_diagnostics.hal_status = (uint8_t)status;
  imu_diagnostics.transfer_ok = (status == HAL_OK) ? 1U : 0U;
  return (status == HAL_OK) ? 1U : 0U;
}

static int16_t Imu660rc_DecodeS16(const uint8_t *data)
{
  return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

uint8_t Imu660rc_ReadWhoAmI(uint8_t *who_am_i)
{
  return Imu660rc_ReadRegisters(IMU660RC_REG_WHO_AM_I, who_am_i, 1U);
}

uint8_t Imu660rc_ReadStatus(uint8_t *status)
{
  return Imu660rc_ReadRegisters(IMU660RC_REG_STATUS, status, 1U);
}

uint8_t Imu660rc_Init(void)
{
  uint8_t who_am_i = 0U;
  uint8_t attempt;

  Imu660rc_Deselect();
  /* The module contains an LDO and the sensor needs a short settling time
   * after the MCU has released CS.  Retry here so a cold power-up does not
   * look like a permanent bus failure. */
  HAL_Delay(10U);
  for (attempt = 0U; attempt < 3U; ++attempt)
  {
    (void)Imu660rc_ReadWhoAmI(&who_am_i);
    if (who_am_i == IMU660RC_WHO_AM_I_VALUE)
    {
      break;
    }
    HAL_Delay(2U);
  }
  if (who_am_i != IMU660RC_WHO_AM_I_VALUE)
  {
    return 0U;
  }

  /* Reboot the device, enable block-data-update and address auto-increment. */
  if (Imu660rc_WriteRegister(IMU660RC_REG_CTRL3, 0x01U) == 0U)
  {
    return 0U;
  }
  HAL_Delay(10U);
  if (Imu660rc_WriteRegister(IMU660RC_REG_CTRL3, 0x44U) == 0U)
  {
    return 0U;
  }

  /* 120 Hz high-performance accelerometer and gyroscope output. */
  if ((Imu660rc_WriteRegister(IMU660RC_REG_CTRL1, 0x06U) == 0U) ||
      (Imu660rc_WriteRegister(IMU660RC_REG_CTRL2, 0x06U) == 0U) ||
      /* Gyro: +/-500 dps, LPF1 disabled. */
      (Imu660rc_WriteRegister(IMU660RC_REG_CTRL6, 0x02U) == 0U) ||
      /* Accelerometer: +/-4 g, LPF2 disabled. */
      (Imu660rc_WriteRegister(IMU660RC_REG_CTRL8, 0x01U) == 0U))
  {
    return 0U;
  }

  return 1U;
}

uint8_t Imu660rc_ReadRaw(Imu660rcAxis3 *gyro, Imu660rcAxis3 *accel)
{
  uint8_t raw[12];

  if ((gyro == NULL) || (accel == NULL) ||
      (Imu660rc_ReadRegisters(IMU660RC_REG_OUT_G, raw, sizeof(raw)) == 0U))
  {
    return 0U;
  }

  gyro->x = Imu660rc_DecodeS16(&raw[0]);
  gyro->y = Imu660rc_DecodeS16(&raw[2]);
  gyro->z = Imu660rc_DecodeS16(&raw[4]);
  accel->x = Imu660rc_DecodeS16(&raw[6]);
  accel->y = Imu660rc_DecodeS16(&raw[8]);
  accel->z = Imu660rc_DecodeS16(&raw[10]);
  return 1U;
}

void Imu660rc_GetDiagnostics(Imu660rcDiagnostics *diagnostics)
{
  if (diagnostics != NULL)
  {
    *diagnostics = imu_diagnostics;
  }
}
