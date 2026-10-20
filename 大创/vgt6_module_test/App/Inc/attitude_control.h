#ifndef ATTITUDE_CONTROL_H
#define ATTITUDE_CONTROL_H
#include <stdint.h>
typedef struct
{
  uint8_t mode; /* 0 stopped, 1 angular velocity PI, 2 heading PD -> PI */
  uint8_t saturated;
  float target_dps, feedback_dps, output_rpm;
  float target_deg, feedback_deg, error_deg, output_dps;
  float rate_kp, rate_ki, heading_kp, heading_kd;
  int16_t forward_rpm, left_rpm;
} AttitudeControlSnapshot;
#define ATTITUDE_PERIOD_MS 20U
#define ATTITUDE_RATE_LIMIT_DPS 80.0f
#define ATTITUDE_OUTPUT_LIMIT_RPM 60.0f
void AttitudeControl_Init(void);
void AttitudeControl_Stop(void);
void AttitudeControl_Process(uint32_t elapsed_ms);
uint8_t AttitudeControl_SetRate(float rate_dps, int16_t forward, int16_t left);
/* Joystick rate shapes a continuous heading reference with rate feedforward.
 * Release cancels angle tracking, damps actual rate without integral, then
 * captures the settled heading; wheel loops are not restarted. */
uint8_t AttitudeControl_SetManualDrive(float rate_dps,
                                      int16_t forward, int16_t left);
uint8_t AttitudeControl_SetHeading(float heading_deg);
uint8_t AttitudeControl_SetHeadingWithDrive(float heading_deg,
                                            int16_t forward,
                                            int16_t left);
/* Supervised motion: translation in the IMU's fixed yaw frame (RPM-equivalent),
 * transformed using measured yaw every 20 ms. Adds quiet heading hold and a
 * bounded translation ramp; raw HEAD/MOVE tuning interfaces remain unchanged. */
uint8_t AttitudeControl_SetTrackedDrive(float heading_deg,
                                        float field_forward, float field_left);
uint8_t AttitudeControl_KeepAlive(void);
uint8_t AttitudeControl_SetRatePid(float kp, float ki);
uint8_t AttitudeControl_SetHeadingPid(float kp, float kd);
AttitudeControlSnapshot AttitudeControl_GetSnapshot(void);
#endif
