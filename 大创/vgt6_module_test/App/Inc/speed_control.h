#ifndef SPEED_CONTROL_H
#define SPEED_CONTROL_H

#include <stdint.h>

#include "motor.h"

#define SPEED_CONTROL_CHANNEL_COUNT 4U
/* BM50 19:1 gearbox: rated shaft speed 4462/19 ~= 235 RPM.
 * Use 200 RPM as a conservative target ceiling, not a measured overspeed trip. */
#define SPEED_CONTROL_MAX_TARGET_RPM 200U
#define SPEED_CONTROL_DEFAULT_KP 1.2f
#define SPEED_CONTROL_DEFAULT_KI 0.0991f /* A; B/C=0.0998, D=0.1008 */
#define SPEED_CONTROL_DEFAULT_KD 0.0f
#define SPEED_CONTROL_MAX_KP 100.0f
#define SPEED_CONTROL_MAX_KI 100.0f
#define SPEED_CONTROL_MAX_KD 100.0f

typedef struct
{
  int32_t target_rpm[SPEED_CONTROL_CHANNEL_COUNT];
  int32_t actual_rpm[SPEED_CONTROL_CHANNEL_COUNT];
  int32_t actual_rpm_x10[SPEED_CONTROL_CHANNEL_COUNT];
  int32_t output_percent[SPEED_CONTROL_CHANNEL_COUNT];
  int32_t output_mpercent[SPEED_CONTROL_CHANNEL_COUNT];
  float kp[SPEED_CONTROL_CHANNEL_COUNT];
  float ki[SPEED_CONTROL_CHANNEL_COUNT];
  float kd[SPEED_CONTROL_CHANNEL_COUNT];
  uint16_t target_limit_rpm;
  uint8_t enabled;
  uint8_t armed;
  uint8_t saturated_mask;
  uint8_t tof_limited;
} SpeedControlSnapshot;

void SpeedControl_Init(void);
void SpeedControl_Process(uint32_t elapsed_ms);
void SpeedControl_Stop(void);
uint8_t SpeedControl_KeepAlive(void);
uint8_t SpeedControl_DriveKeepAlive(void);
uint8_t SpeedControl_Enable(uint8_t enable);
uint8_t SpeedControl_SetTarget(MotorChannel channel, int32_t rpm);
uint8_t SpeedControl_SetDrive(int16_t forward_rpm,
                              int16_t left_rpm,
                              int16_t rotate_rpm);
/* R12 wheel gains are runtime-tunable through PID + PIDGET with readback. */
uint8_t SpeedControl_SetPid(MotorChannel channel, float kp, float ki,
                            float kd);
uint8_t SpeedControl_SetTargetLimit(uint16_t limit_rpm);
SpeedControlSnapshot SpeedControl_GetSnapshot(void);

#endif /* SPEED_CONTROL_H */
