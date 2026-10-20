#include "attitude_filter.h"
#include <math.h>
#include <string.h>
#define RAD 0.0174532925199433f
#define MAHONY_KP                 2.0f
#define MAHONY_KI                 0.08f
#define MAHONY_INTEGRAL_LIMIT     0.35f
#define ACCEL_NORM_VALID_MIN      0.70f
#define ACCEL_NORM_VALID_MAX      1.30f

static float AttitudeFilter_Clamp(float value, float limit)
{
  if (value > limit) return limit;
  if (value < -limit) return -limit;
  return value;
}

/*
 * Six-axis adaptive Mahony observer:
 * - accelerometer correction is weighted by |a| confidence, so chassis
 *   acceleration does not inject a full tilt correction;
 * - the integral feedback learns residual tilt gyro bias while gravity is
 *   trustworthy;
 * - yaw remains gyro-integrated because this board has no magnetometer.
 * Gyro bias is also learned by Gyro_Process during confirmed ZUPT rest.
 */
void AttitudeFilter_Init(AttitudeFilter *s, const float a[3])
{
  float roll = atan2f(a[1], a[2]);
  float pitch = atan2f(-a[0], sqrtf(a[1]*a[1] + a[2]*a[2]));
  float cr = cosf(roll/2), sr = sinf(roll/2);
  float cp = cosf(pitch/2), sp = sinf(pitch/2);
  memset(s, 0, sizeof(*s));
  s->q[0] = cr*cp; s->q[1] = sr*cp;
  s->q[2] = cr*sp; s->q[3] = -sr*sp;
  s->accel_confidence = 1.0f;
}
void AttitudeFilter_Update(AttitudeFilter *s, const float g[3],
                            const float a[3], float dt)
{
  float w=s->q[0], x=s->q[1], y=s->q[2], z=s->q[3];
  float gx=g[0]*RAD, gy=g[1]*RAD, gz=g[2]*RAD;
  float norm=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
  float yaw, roll, pitch, inv, error_x, error_y, error_z;
  float confidence;
  if (!isfinite(dt) || dt <= 0 || dt > 0.1f) return;
  confidence = 0.0f;
  if (norm > ACCEL_NORM_VALID_MIN && norm < ACCEL_NORM_VALID_MAX)
  {
    float ax=a[0]/norm, ay=a[1]/norm, az=a[2]/norm;
    float vx=2*(x*z-w*y), vy=2*(w*x+y*z), vz=w*w-x*x-y*y+z*z;
    confidence = 1.0f - fabsf(norm - 1.0f) / 0.30f;
    confidence = fmaxf(0.0f, fminf(1.0f, confidence));
    error_x = ay*vz-az*vy;
    error_y = az*vx-ax*vz;
    error_z = ax*vy-ay*vx;
    s->integral_fb[0] = AttitudeFilter_Clamp(
        s->integral_fb[0] + MAHONY_KI * confidence * error_x * dt,
        MAHONY_INTEGRAL_LIMIT);
    s->integral_fb[1] = AttitudeFilter_Clamp(
        s->integral_fb[1] + MAHONY_KI * confidence * error_y * dt,
        MAHONY_INTEGRAL_LIMIT);
    s->integral_fb[2] = AttitudeFilter_Clamp(
        s->integral_fb[2] + MAHONY_KI * confidence * error_z * dt,
        MAHONY_INTEGRAL_LIMIT);
    gx += MAHONY_KP * confidence * error_x + s->integral_fb[0];
    gy += MAHONY_KP * confidence * error_y + s->integral_fb[1];
    gz += MAHONY_KP * confidence * error_z + s->integral_fb[2];
    s->accel_confidence = confidence;
  }
  else
  {
    /* Do not retain a stale integral correction through a hard acceleration. */
    confidence = 0.0f;
    s->integral_fb[0] *= fmaxf(0.0f, 1.0f - 2.0f * dt);
    s->integral_fb[1] *= fmaxf(0.0f, 1.0f - 2.0f * dt);
    s->integral_fb[2] *= fmaxf(0.0f, 1.0f - 2.0f * dt);
    s->accel_confidence = 0.0f;
  }
  s->q[0] += 0.5f*(-x*gx-y*gy-z*gz)*dt;
  s->q[1] += 0.5f*( w*gx+y*gz-z*gy)*dt;
  s->q[2] += 0.5f*( w*gy-x*gz+z*gx)*dt;
  s->q[3] += 0.5f*( w*gz+x*gy-y*gx)*dt;
  inv=1/sqrtf(s->q[0]*s->q[0]+s->q[1]*s->q[1]+
              s->q[2]*s->q[2]+s->q[3]*s->q[3]);
  if (!isfinite(inv)) return;
  s->q[0]*=inv; s->q[1]*=inv; s->q[2]*=inv; s->q[3]*=inv;
  w=s->q[0]; x=s->q[1]; y=s->q[2]; z=s->q[3];
  roll=atan2f(2*(w*x+y*z), 1-2*(x*x+y*y));
  pitch=asinf(fmaxf(-1, fminf(1, 2*(w*y-z*x))));
  yaw=atan2f(2*(w*z+x*y), 1-2*(y*y+z*z));
  s->roll_deg=roll/RAD; s->pitch_deg=pitch/RAD; s->yaw_deg=yaw/RAD;
  s->rate_dps=(g[1]*sinf(roll)+g[2]*cosf(roll))/
               fmaxf(0.173648f, cosf(pitch));
}
