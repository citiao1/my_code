#include "vehicle_comm.h"

#include "vehicle_config.h"
#include "vehicle_control.h"
#include "vehicle_internal.h"
#include "vehicle_line.h"
#include "vehicle_motor.h"

#include <stdio.h>
#include <string.h>

static uint8_t uart_rx_byte;
static volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t uart_rx_head;
static volatile uint16_t uart_rx_tail;
static char uart_line[UART_LINE_SIZE];
static uint8_t uart_line_length;
static char uart_tx_buffer[384];
static volatile uint8_t uart_tx_busy;
static uint8_t calibration_report_pending;

static int8_t ClampPercent(int value)
{
  if (value > 100) return 100;
  if (value < -100) return -100;
  return (int8_t)value;
}

void VehicleComm_Init(void)
{
  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
}

static void TrySendCalibrationReport(void)
{
  int length;
  if (!calibration_report_pending || uart_tx_busy) return;

  length = snprintf(uart_tx_buffer, sizeof(uart_tx_buffer),
                    "CAL,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                    (unsigned int)line_follow.gray_white[0],
                    (unsigned int)line_follow.gray_white[1],
                    (unsigned int)line_follow.gray_white[2],
                    (unsigned int)line_follow.gray_white[3],
                    (unsigned int)line_follow.gray_white[4],
                    (unsigned int)line_follow.gray_white[5],
                    (unsigned int)line_follow.gray_white[6],
                    (unsigned int)line_follow.gray_white[7],
                    (unsigned int)line_follow.gray_black[0],
                    (unsigned int)line_follow.gray_black[1],
                    (unsigned int)line_follow.gray_black[2],
                    (unsigned int)line_follow.gray_black[3],
                    (unsigned int)line_follow.gray_black[4],
                    (unsigned int)line_follow.gray_black[5],
                    (unsigned int)line_follow.gray_black[6],
                    (unsigned int)line_follow.gray_black[7]);
  if (length <= 0 || length >= (int)sizeof(uart_tx_buffer)) return;

  uart_tx_busy = 1U;
  if (HAL_UART_Transmit_IT(&huart2, (uint8_t *)uart_tx_buffer, (uint16_t)length) == HAL_OK)
  {
    calibration_report_pending = 0U;
  }
  else
  {
    uart_tx_busy = 0U;
  }
}

uint8_t VehicleComm_IsTxBusy(void)
{
  return uart_tx_busy;
}

void VehicleComm_SendTelemetry(uint32_t now)
{
  int length;
  if (uart_tx_busy) return;

  length = snprintf(uart_tx_buffer, sizeof(uart_tx_buffer),
                    "TEL,%lu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%d,%d,%d,%d,%u,%u,%d,%d\n",
                    (unsigned long)now, VehicleMotor_IsEnabled(), state.link_active,
                    (int)(state.yaw * 10.0f), (int)(state.speed_left * 1000.0f),
                    (int)(state.speed_right * 1000.0f), (int)(state.target_left * 1000.0f),
                    (int)(state.target_right * 1000.0f), state.pwm_left, state.pwm_right,
                    (int)(state.target_yaw_rate * 10.0f), (int)(state.yaw_rate * 10.0f),
                    (int)(state.yaw_error * 10.0f), (int)(state.yaw_correction * 1000.0f),
                    state.yaw_control_enabled, state.mpu_ok,
                    (int)(state.yaw_feedforward * 1000.0f),
                    (int)(state.target_heading * 10.0f), (int)(state.heading_error * 10.0f),
                    (int)(state.heading_output * 10.0f), state.heading_control_enabled,
                    state.heading_hold_active, (int)(state.battery_voltage * 1000.0f));
  if (length <= 0 || length >= (int)sizeof(uart_tx_buffer)) return;

  uart_tx_busy = 1U;
  if (HAL_UART_Transmit_IT(&huart2, (uint8_t *)uart_tx_buffer, (uint16_t)length) != HAL_OK)
  {
    uart_tx_busy = 0U;
  }
}

void VehicleComm_SendStatus(uint32_t now)
{
  int length;
  int appended;
  if (uart_tx_busy) return;

  length = snprintf(uart_tx_buffer, sizeof(uart_tx_buffer),
                    "STA,%lu,%d,%d,%ld,%ld,%u,%d,%d,%d,%d,%d,%d,%u,%u,%u,%d,%d,%d,"
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
                    (unsigned long)now, (int)(state.pitch * 10.0f), (int)(state.roll * 10.0f),
                    (long)state.encoder_left, (long)state.encoder_right, state.battery_raw,
                    (int)pid_left.kp, (int)pid_left.ki, (int)pid_left.kd,
                    (int)pid_right.kp, (int)pid_right.ki, (int)pid_right.kd,
                    square_test.active, (unsigned int)square_test.phase,
                    (unsigned int)(square_test.leg < 4U ? square_test.leg + 1U : 4U),
                    (int)square_test.direction,
                    (int)(square_test.progress_m * 1000.0f),
                    (int)((SQUARE_SIDE_DISTANCE_M - square_test.progress_m) * 1000.0f),
                    (unsigned int)line_follow.phase,
                    (unsigned int)VehicleLine_GetSpeedPercent(),
                    (unsigned int)line_follow.active_count,
                    (unsigned int)VehicleLine_GetCornerAdvanceMm(),
                    (unsigned int)line_follow.gray[0], (unsigned int)line_follow.gray[1],
                    (unsigned int)line_follow.gray[2], (unsigned int)line_follow.gray[3],
                    (unsigned int)line_follow.gray[4], (unsigned int)line_follow.gray[5],
                    (unsigned int)line_follow.gray[6], (unsigned int)line_follow.gray[7],
                    (unsigned int)VehicleLine_GetCornerTurnDeg(),
                    (unsigned int)line_follow.corner_contiguous_count);
  if (length <= 0 || length >= (int)sizeof(uart_tx_buffer)) return;

  appended = snprintf(uart_tx_buffer + length, sizeof(uart_tx_buffer) - (size_t)length,
                      ",%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u\n",
                      (unsigned int)line_follow.white_calibrated,
                      (unsigned int)line_follow.black_calibrated,
                      (int)(VehicleLine_GetDirectionKp() * 1000.0f),
                      (int)(VehicleLine_GetDirectionKd() * 1000.0f),
                      (unsigned int)line_follow.gray_normalized[0],
                      (unsigned int)line_follow.gray_normalized[1],
                      (unsigned int)line_follow.gray_normalized[2],
                      (unsigned int)line_follow.gray_normalized[3],
                      (unsigned int)line_follow.gray_normalized[4],
                      (unsigned int)line_follow.gray_normalized[5],
                      (unsigned int)line_follow.gray_normalized[6],
                      (unsigned int)line_follow.gray_normalized[7]);
  if (appended <= 0 || length + appended >= (int)sizeof(uart_tx_buffer)) return;
  length += appended;

  uart_tx_busy = 1U;
  if (HAL_UART_Transmit_IT(&huart2, (uint8_t *)uart_tx_buffer, (uint16_t)length) != HAL_OK)
  {
    uart_tx_busy = 0U;
  }
}

static void ProcessLine(char *line)
{
  int throttle;
  int steering;
  int kp;
  int ki;
  int kd;
  int max_speed_mm;
  int yaw_enable;
  int max_yaw_rate;
  int yaw_kp_micro;
  int yaw_ki_micro;
  int yaw_kd_micro;
  int yaw_kff_micro;
  int yaw_pid_fields;
  int heading_enable;
  int heading_kp_milli;
  int heading_kd_milli;
  int heading_kff_milli;
  int heading_max_rate;
  int heading_pid_fields;
  int heading_target10;
  int square_throttle;
  int square_direction;
  int line_speed_percent;
  int line_kp_milli;
  int line_kd_milli;
  int line_corner_advance_mm;
  int line_corner_turn_deg;

  if (sscanf(line, "DRV,%d,%d", &throttle, &steering) == 2)
  {
    if (VehicleLine_IsEngaged()) VehicleLine_Stop();
    if (square_test.active) VehicleControl_CancelSquare(SQUARE_PHASE_IDLE);
    state.throttle = ClampPercent(throttle);
    state.steering = ClampPercent(steering);
    last_command_ms = HAL_GetTick();
  }
  else if (sscanf(line, "PIDL,%d,%d,%d", &kp, &ki, &kd) == 3)
  {
    if (kp >= 0 && kp <= 50000 && ki >= 0 && ki <= 50000 && kd >= 0 && kd <= 5000)
    {
      pid_left.kp = (float)kp;
      pid_left.ki = (float)ki;
      pid_left.kd = (float)kd;
      VehicleMotor_Stop();
    }
  }
  else if (sscanf(line, "PIDR,%d,%d,%d", &kp, &ki, &kd) == 3)
  {
    if (kp >= 0 && kp <= 50000 && ki >= 0 && ki <= 50000 && kd >= 0 && kd <= 5000)
    {
      pid_right.kp = (float)kp;
      pid_right.ki = (float)ki;
      pid_right.kd = (float)kd;
      VehicleMotor_Stop();
    }
  }
  else if (sscanf(line, "PID,%d,%d,%d", &kp, &ki, &kd) == 3)
  {
    if (kp >= 0 && kp <= 50000 && ki >= 0 && ki <= 50000 && kd >= 0 && kd <= 5000)
    {
      pid_left.kp = pid_right.kp = (float)kp;
      pid_left.ki = pid_right.ki = (float)ki;
      pid_left.kd = pid_right.kd = (float)kd;
      VehicleMotor_Stop();
    }
  }
  else if (sscanf(line, "MAX,%d", &max_speed_mm) == 1)
  {
    state.max_speed = MOTOR_SPEED_LIMIT_MPS;
  }
  else if (sscanf(line, "LINESPD,%d", &line_speed_percent) == 1)
  {
    if (line_speed_percent >= 20 && line_speed_percent <= 100)
      VehicleLine_SetSpeedPercent((uint8_t)line_speed_percent);
  }
  else if (sscanf(line, "LINEPID,%d,%d", &line_kp_milli, &line_kd_milli) == 2)
  {
    if (line_kp_milli >= 0 && line_kp_milli <= (int)(LINE_DIRECTION_KP_MAX * 1000.0f) &&
        line_kd_milli >= 0 && line_kd_milli <= (int)(LINE_DIRECTION_KD_MAX * 1000.0f))
      VehicleLine_SetDirectionGains((float)line_kp_milli / 1000.0f,
                                    (float)line_kd_milli / 1000.0f);
  }
  else if (strcmp(line, "GRAYWHITE") == 0)
  {
    VehicleLine_Stop();
    VehicleLine_CaptureWhite();
    calibration_report_pending = 1U;
  }
  else if (strcmp(line, "GRAYBLACK") == 0)
  {
    VehicleLine_Stop();
    VehicleLine_CaptureBlack();
    calibration_report_pending = 1U;
  }
  else if (strcmp(line, "GRAYCAL") == 0)
  {
    calibration_report_pending = 1U;
  }
  else if (sscanf(line, "LINEADV,%d", &line_corner_advance_mm) == 1)
  {
    if (line_corner_advance_mm >= (int)LINE_CORNER_ADVANCE_MIN_MM &&
        line_corner_advance_mm <= (int)LINE_CORNER_ADVANCE_MAX_MM)
      VehicleLine_SetCornerAdvanceMm((uint16_t)line_corner_advance_mm);
  }
  else if (sscanf(line, "LINETURN,%d", &line_corner_turn_deg) == 1)
  {
    if (line_corner_turn_deg >= (int)LINE_CORNER_TURN_MIN_DEG &&
        line_corner_turn_deg <= (int)LINE_CORNER_TURN_MAX_DEG)
      VehicleLine_SetCornerTurnDeg((uint16_t)line_corner_turn_deg);
  }
  else if (sscanf(line, "YAW,%d", &yaw_enable) == 1)
  {
    state.yaw_control_enabled = (yaw_enable != 0 && state.mpu_ok) ? 1U : 0U;
    if (!state.yaw_control_enabled) state.heading_control_enabled = 0U;
    VehicleMotor_Stop();
  }
  else if (sscanf(line, "YAWRATE,%d", &max_yaw_rate) == 1)
  {
    if (max_yaw_rate >= 10 && max_yaw_rate <= 360) state.max_yaw_rate = (float)max_yaw_rate;
  }
  else if (strncmp(line, "YAWPID,", 7U) == 0)
  {
    yaw_pid_fields = sscanf(line, "YAWPID,%d,%d,%d,%d", &yaw_kp_micro, &yaw_ki_micro,
                            &yaw_kd_micro, &yaw_kff_micro);
    if ((yaw_pid_fields == 3 || yaw_pid_fields == 4) &&
        yaw_kp_micro >= 0 && yaw_kp_micro <= 100000 &&
        yaw_ki_micro >= 0 && yaw_ki_micro <= 100000 &&
        yaw_kd_micro >= 0 && yaw_kd_micro <= 100000 &&
        (yaw_pid_fields == 3 || (yaw_kff_micro >= 0 && yaw_kff_micro <= 100000)))
    {
      pid_yaw.kp = (float)yaw_kp_micro / 1000000.0f;
      pid_yaw.ki = (float)yaw_ki_micro / 1000000.0f;
      pid_yaw.kd = (float)yaw_kd_micro / 1000000.0f;
      if (yaw_pid_fields == 4) pid_yaw.kff = (float)yaw_kff_micro / 1000000.0f;
      VehicleMotor_Stop();
    }
  }
  else if (strncmp(line, "HEADPID,", 8U) == 0)
  {
    heading_pid_fields = sscanf(line, "HEADPID,%d,%d,%d,%d", &heading_kp_milli,
                                &heading_kd_milli, &heading_kff_milli, &heading_max_rate);
    if (heading_pid_fields == 3)
    {
      heading_max_rate = heading_kff_milli;
      heading_kff_milli = (int)(pid_heading.kff * 1000.0f);
    }
    if ((heading_pid_fields == 3 || heading_pid_fields == 4) &&
        heading_kp_milli >= 0 && heading_kp_milli <= 20000 &&
        heading_kd_milli >= 0 && heading_kd_milli <= 10000 &&
        heading_kff_milli >= 0 && heading_kff_milli <= 2000 &&
        heading_max_rate >= 5 && heading_max_rate <= 360)
    {
      pid_heading.kp = (float)heading_kp_milli / 1000.0f;
      pid_heading.kd = (float)heading_kd_milli / 1000.0f;
      pid_heading.kff = (float)heading_kff_milli / 1000.0f;
      state.max_heading_yaw_rate = (float)heading_max_rate;
      state.heading_hold_active = 0U;
      VehicleMotor_Stop();
    }
  }
  else if (sscanf(line, "HEADSET,%d", &heading_target10) == 1)
  {
    if (heading_target10 >= -1800 && heading_target10 <= 1800 && state.heading_control_enabled)
    {
      state.target_heading = (float)heading_target10 / 10.0f;
      state.heading_error = Vehicle_WrapAngle(state.target_heading - state.yaw);
      state.heading_hold_active = 1U;
      last_heading_ms = HAL_GetTick();
    }
  }
  else if (sscanf(line, "HEAD,%d", &heading_enable) == 1)
  {
    state.heading_control_enabled =
      (heading_enable != 0 && state.yaw_control_enabled && state.mpu_ok) ? 1U : 0U;
    state.target_heading = state.yaw;
    state.heading_reference = state.yaw;
    state.heading_reference_rate = 0.0f;
    state.heading_hold_active = state.heading_control_enabled;
    state.heading_error = 0.0f;
    state.heading_output = 0.0f;
    last_heading_ms = HAL_GetTick();
    VehicleMotor_Stop();
  }
  else if (sscanf(line, "SQUARE,%d,%d", &square_throttle, &square_direction) == 2)
  {
    if (square_throttle >= 10 && square_throttle <= 60 &&
        (square_direction == -1 || square_direction == 1))
    {
      if (VehicleLine_IsEngaged()) VehicleLine_Stop();
      VehicleControl_StartSquare((uint8_t)square_throttle, (int8_t)square_direction, HAL_GetTick());
    }
  }
  else if (strcmp(line, "STOP") == 0)
  {
    VehicleLine_Stop();
    if (square_test.active) square_test.phase = SQUARE_PHASE_IDLE;
    square_test.active = 0U;
    state.throttle = 0;
    state.steering = 0;
    last_command_ms = 0;
    state.link_active = 0;
    VehicleMotor_Stop();
  }
  else if (strcmp(line, "PING") == 0)
  {
    last_command_ms = HAL_GetTick();
  }
  else if (strcmp(line, "ZERO") == 0)
  {
    if (square_test.active) VehicleControl_CancelSquare(SQUARE_PHASE_IDLE);
    state.encoder_left = 0;
    state.encoder_right = 0;
    state.yaw = 0.0f;
    state.target_heading = 0.0f;
    state.heading_reference = 0.0f;
    state.heading_reference_rate = 0.0f;
    state.heading_error = 0.0f;
  }
}

void VehicleComm_Process(void)
{
  while (uart_rx_tail != uart_rx_head)
  {
    char byte = (char)uart_rx_buffer[uart_rx_tail];
    uart_rx_tail = (uint16_t)((uart_rx_tail + 1U) % UART_RX_BUFFER_SIZE);

    if (byte == '\n' || byte == '\r')
    {
      if (uart_line_length > 0U)
      {
        uart_line[uart_line_length] = '\0';
        ProcessLine(uart_line);
        uart_line_length = 0U;
      }
    }
    else if (uart_line_length < UART_LINE_SIZE - 1U)
    {
      uart_line[uart_line_length++] = byte;
    }
    else
    {
      uart_line_length = 0U;
    }
  }
  TrySendCalibrationReport();
}

void VehicleComm_RxCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    uint16_t next = (uint16_t)((uart_rx_head + 1U) % UART_RX_BUFFER_SIZE);
    if (next != uart_rx_tail)
    {
      uart_rx_buffer[uart_rx_head] = uart_rx_byte;
      uart_rx_head = next;
    }
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}

void VehicleComm_TxCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2) uart_tx_busy = 0U;
}

void VehicleComm_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    uart_tx_busy = 0U;
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}
