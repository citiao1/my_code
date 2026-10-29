#ifndef FRONT_GUARD_H
#define FRONT_GUARD_H
#include <stdint.h>
#define FRONT_GUARD_STOP_MM 100U
#define FRONT_GUARD_SLOW_MM 200U
#define FRONT_GUARD_SLOW_RPM 40U
/* New front is the TOF end (180 degrees from the original body frame).
 * Invalid/stale TOF intentionally leaves speed unchanged, per owner policy. */
uint16_t FrontGuard_ForwardLimit(uint16_t maximum_rpm);
#endif
