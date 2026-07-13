#ifndef VEHICLE_TYPES_H
#define VEHICLE_TYPES_H

#include <stdint.h>

typedef struct
{
  float previous_error;
  float previous_previous_error;
  float output;
  float kp;
  float ki;
  float kd;
} SpeedPidState;

typedef struct
{
  float integral;
  float previous_error;
  float kp;
  float ki;
  float kd;
  float kff;
} YawPidState;

typedef struct
{
  float kp;
  float kd;
  float kff;
} HeadingPidState;

typedef enum
{
  SQUARE_PHASE_IDLE = 0,
  SQUARE_PHASE_DRIVE = 1,
  SQUARE_PHASE_TURN = 2,
  SQUARE_PHASE_COMPLETE = 3,
  SQUARE_PHASE_ERROR = 4
} SquarePhase;

typedef struct
{
  int32_t start_left;
  int32_t start_right;
  float baseline_heading;
  float progress_m;
  uint32_t phase_start_ms;
  uint32_t settle_start_ms;
  int8_t direction;
  uint8_t max_throttle;
  uint8_t leg;
  uint8_t active;
  SquarePhase phase;
} SquareTestState;

typedef struct
{
  float speed_left;
  float speed_right;
  float target_left;
  float target_right;
  float error_left;
  float error_right;
  float max_speed;
  float pitch;
  float roll;
  float yaw;
  float yaw_rate;
  float target_yaw_rate;
  float yaw_error;
  float yaw_correction;
  float yaw_feedforward;
  float max_yaw_rate;
  float target_heading;
  float heading_reference;
  float heading_reference_rate;
  float heading_error;
  float heading_output;
  float max_heading_yaw_rate;
  float battery_voltage;
  float gyro_bias_x;
  float gyro_bias_y;
  float gyro_bias_z;
  int32_t encoder_left;
  int32_t encoder_right;
  int16_t pwm_left;
  int16_t pwm_right;
  uint16_t battery_raw;
  int8_t throttle;
  int8_t steering;
  uint8_t mpu_ok;
  uint8_t link_active;
  uint8_t yaw_control_enabled;
  uint8_t heading_control_enabled;
  uint8_t heading_hold_active;
} VehicleState;

#endif
