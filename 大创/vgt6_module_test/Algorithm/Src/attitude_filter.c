#include "attitude_filter.h"
#include <math.h>
#include <string.h>
#define RAD 0.0174532925199433f
/* Six-axis nonlinear complementary observer (Mahony et al., 2008).
 * Acceleration corrects tilt only. No magnetic/absolute yaw observation.
 * Gyro bias is estimated separately, exclusively during confirmed rest. */
void AttitudeFilter_Init(AttitudeFilter *s, const float a[3])
{
  float roll = atan2f(a[1], a[2]);
  float pitch = atan2f(-a[0], sqrtf(a[1]*a[1] + a[2]*a[2]));
  float cr = cosf(roll/2), sr = sinf(roll/2);
  float cp = cosf(pitch/2), sp = sinf(pitch/2);
  memset(s, 0, sizeof(*s));
  s->q[0] = cr*cp; s->q[1] = sr*cp;
  s->q[2] = cr*sp; s->q[3] = -sr*sp;
}
void AttitudeFilter_Update(AttitudeFilter *s, const float g[3],
                            const float a[3], float dt)
{
  float w=s->q[0], x=s->q[1], y=s->q[2], z=s->q[3];
  float gx=g[0]*RAD, gy=g[1]*RAD, gz=g[2]*RAD;
  float norm=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
  float yaw, roll, pitch, inv;
  if (!isfinite(dt) || dt <= 0 || dt > 0.1f) return;
  if (norm > 0.85f && norm < 1.15f)
  {
    float ax=a[0]/norm, ay=a[1]/norm, az=a[2]/norm;
    float vx=2*(x*z-w*y), vy=2*(w*x+y*z), vz=w*w-x*x-y*y+z*z;
    gx += 2.0f*(ay*vz-az*vy);
    gy += 2.0f*(az*vx-ax*vz);
    gz += 2.0f*(ax*vy-ay*vx);
  }
  s->q[0] += 0.5f*(-x*gx-y*gy-z*gz)*dt;
  s->q[1] += 0.5f*( w*gx+y*gz-z*gy)*dt;
  s->q[2] += 0.5f*( w*gy-x*gz+z*gx)*dt;
  s->q[3] += 0.5f*( w*gz+x*gy-y*gx)*dt;
  inv=1/sqrtf(s->q[0]*s->q[0]+s->q[1]*s->q[1]+
              s->q[2]*s->q[2]+s->q[3]*s->q[3]);
  s->q[0]*=inv; s->q[1]*=inv; s->q[2]*=inv; s->q[3]*=inv;
  w=s->q[0]; x=s->q[1]; y=s->q[2]; z=s->q[3];
  roll=atan2f(2*(w*x+y*z), 1-2*(x*x+y*y));
  pitch=asinf(fmaxf(-1, fminf(1, 2*(w*y-z*x))));
  yaw=atan2f(2*(w*z+x*y), 1-2*(y*y+z*z));
  s->roll_deg=roll/RAD; s->pitch_deg=pitch/RAD; s->yaw_deg=yaw/RAD;
  s->rate_dps=(g[1]*sinf(roll)+g[2]*cosf(roll))/
               fmaxf(0.173648f, cosf(pitch));
}
