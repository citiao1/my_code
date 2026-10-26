#ifndef TOF_FOLLOW_H
#define TOF_FOLLOW_H

#include <stdint.h>
#include "tof.h"

typedef enum { TOF_FOLLOW_OFF, TOF_FOLLOW_TRACK, TOF_FOLLOW_SEARCH } TofFollowMode;
typedef enum
{
  TOF_FOLLOW_NORMAL, TOF_FOLLOW_CANCELLED, TOF_FOLLOW_TIMEOUT,
  TOF_FOLLOW_SENSOR_FAULT, TOF_FOLLOW_TOO_CLOSE, TOF_FOLLOW_CONTROL_FAULT
} TofFollowReason;

typedef struct
{
  TofFollowMode mode;
  TofFollowReason reason;
  uint16_t filtered_mm;
  int16_t forward_rpm;
  float turn_dps;
} TofFollowSnapshot;

void TofFollow_Init(void);
uint8_t TofFollow_Start(const TofSnapshot *range, float yaw_deg, uint32_t now);
void TofFollow_Stop(TofFollowReason reason);
void TofFollow_Process(const TofSnapshot *range, float yaw_deg,
                       float rate_dps, uint32_t now);
TofFollowSnapshot TofFollow_GetSnapshot(void);

#endif
