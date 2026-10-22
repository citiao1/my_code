#include "front_guard.h"
#include "tof.h"

uint16_t FrontGuard_ForwardLimit(uint16_t maximum_rpm)
{
  TofSnapshot tof = Tof_GetSnapshot();
  if (!tof.connected || !tof.valid || tof.age_ms > TOF_MAX_AGE_MS ||
      tof.distance_mm >= 100U) return maximum_rpm;
  if (tof.distance_mm <= 50U) return 0U;
  return (uint16_t)((uint32_t)maximum_rpm * (tof.distance_mm - 50U) / 50U);
}
