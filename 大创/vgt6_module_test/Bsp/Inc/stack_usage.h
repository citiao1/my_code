#ifndef STACK_USAGE_H
#define STACK_USAGE_H

#include <stdint.h>

/* Must match CubeMX ProjectManager.StackSize and the linked startup area. */
#define STACK_USAGE_BUDGET_BYTES 4096U

typedef struct
{
  uint32_t allocated_bytes;
  uint32_t used_bytes;
  uint32_t free_bytes;
  uint8_t guard_ok;
} StackUsageSnapshot;

void StackUsage_Init(void);
uint8_t StackUsage_IsHealthy(void);
StackUsageSnapshot StackUsage_GetSnapshot(void);

#endif
