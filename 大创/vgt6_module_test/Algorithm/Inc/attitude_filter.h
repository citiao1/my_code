#ifndef ATTITUDE_FILTER_H
#define ATTITUDE_FILTER_H
typedef struct
{
  float q[4];
  float integral_fb[3];
  float accel_confidence;
  float yaw_deg, roll_deg, pitch_deg, rate_dps;
} AttitudeFilter;
void AttitudeFilter_Init(AttitudeFilter *s, const float accel_g[3]);
void AttitudeFilter_Update(AttitudeFilter *s, const float gyro_dps[3],
                            const float accel_g[3], float dt);
#endif
