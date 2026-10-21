#ifndef FRONT_GUARD_H
#define FRONT_GUARD_H
#include <stdint.h>
/* New front is the TOF end (180 degrees from the original body frame).
 * Invalid/stale TOF intentionally leaves speed unchanged, per owner policy. */
uint16_t FrontGuard_ForwardLimit(uint16_t maximum_rpm);
#endif
