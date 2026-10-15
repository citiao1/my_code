#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "gyro.h"
#include "imu660rc.h"
#include "attitude_filter.h"
static uint32_t tick;
static Imu660rcAxis3 raw_g = {0,0,50}, raw_a = {0,0,8197};
uint32_t HAL_GetTick(void) { return tick; }
uint8_t Imu660rc_Init(void) { return 1; }
uint8_t Imu660rc_ReadWhoAmI(uint8_t *who) { *who=0x70; return 1; }
uint8_t Imu660rc_ReadStatus(uint8_t *s) { *s=3; return 1; }
uint8_t Imu660rc_ReadRaw(Imu660rcAxis3 *g, Imu660rcAxis3 *a)
{ *g=raw_g; *a=raw_a; return 1; }
void Imu660rc_GetDiagnostics(Imu660rcDiagnostics *d)
{ d->who_am_i=0x70; d->status_reg=3; d->transfer_ok=1; d->hal_status=0; }
static void steps(int n, int stationary)
{ for(int i=0;i<n;i++){ tick+=10; Gyro_Process(0.01f,(uint8_t)stationary); } }
int main(void)
{
  AttitudeFilter s;
  float a[3]={0,0.5f,0.8660254f}, g[3]={0,15,25.980762f};
  AttitudeFilter_Init(&s,a);
  for(int i=0;i<300;i++) AttitudeFilter_Update(&s,g,a,0.01f);
  assert(fabsf(s.yaw_deg-90)<0.1f && fabsf(s.roll_deg-30)<0.1f);
  assert(fabsf(s.rate_dps-30)<0.1f);
  assert(Gyro_Init());
  steps(221,1);
  assert(Gyro_GetSnapshot().ready && fabsf(Gyro_GetSnapshot().bias_z-50)<0.01f);
  raw_g.z=100;
  steps(300,0);
  assert(!Gyro_GetSnapshot().stationary && fabsf(Gyro_GetSnapshot().bias_z-50)<0.01f);
  assert(Gyro_GetSnapshot().yaw_deg>2);
  Gyro_ZeroYaw();
  assert(Gyro_GetSnapshot().yaw_deg==0);
  raw_g.z=60;
  steps(600,1);
  assert(Gyro_GetSnapshot().stationary && Gyro_GetSnapshot().bias_z>50);
  raw_a.z=5000;
  Gyro_StartCalibration();
  steps(250,1);
  assert(!Gyro_GetSnapshot().ready);
  puts("PASS: tilted quaternion yaw, calibration, motor-motion bias guard, acceleration gate");
  return 0;
}
