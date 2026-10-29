#include "front_guard.h"
#include "tof.h"

uint16_t FrontGuard_ForwardLimit(uint16_t maximum_rpm)
{
  TofSnapshot tof = Tof_GetSnapshot();
  uint16_t limit;
  if (!tof.connected || !tof.valid || tof.age_ms > TOF_MAX_AGE_MS ||
      tof.distance_mm > FRONT_GUARD_SLOW_MM) return maximum_rpm;
  if (tof.distance_mm <= FRONT_GUARD_STOP_MM) return 0U;
  limit = (uint16_t)((uint32_t)FRONT_GUARD_SLOW_RPM *
      (tof.distance_mm - FRONT_GUARD_STOP_MM) /
      (FRONT_GUARD_SLOW_MM - FRONT_GUARD_STOP_MM));
  return limit < maximum_rpm ? limit : maximum_rpm;
}
