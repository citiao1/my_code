#ifndef PS2_H
#define PS2_H

#include <stdint.h>

typedef struct
{
  uint8_t connected;
  uint8_t valid;
  uint8_t mode;
  uint8_t right_x;
  uint8_t right_y;
  uint8_t left_x;
  uint8_t left_y;
  uint16_t buttons;
  uint32_t last_tick;
  uint16_t read_failures;
  uint8_t raw_frame[9];
  uint16_t config_attempts;
} Ps2Snapshot;

void Ps2_Init(void);
void Ps2_Process(void);
Ps2Snapshot Ps2_GetSnapshot(void);
uint8_t Ps2_IsPressed(uint16_t button);

#define PS2_BUTTON_SELECT   (1U << 0)
#define PS2_BUTTON_L3       (1U << 1)
#define PS2_BUTTON_R3       (1U << 2)
#define PS2_BUTTON_START    (1U << 3)
#define PS2_BUTTON_UP       (1U << 4)
#define PS2_BUTTON_RIGHT    (1U << 5)
#define PS2_BUTTON_DOWN     (1U << 6)
#define PS2_BUTTON_LEFT     (1U << 7)
#define PS2_BUTTON_L2       (1U << 8)
#define PS2_BUTTON_R2       (1U << 9)
#define PS2_BUTTON_L1       (1U << 10)
#define PS2_BUTTON_R1       (1U << 11)
#define PS2_BUTTON_TRIANGLE (1U << 12)
#define PS2_BUTTON_CIRCLE   (1U << 13)
#define PS2_BUTTON_CROSS    (1U << 14)
#define PS2_BUTTON_SQUARE   (1U << 15)

#endif /* PS2_H */
