#ifndef PS2_H
#define PS2_H

#include <stdint.h>

typedef enum
{
  PS2_BUTTON_SELECT   = (1U << 0),
  PS2_BUTTON_L3       = (1U << 1),
  PS2_BUTTON_R3       = (1U << 2),
  PS2_BUTTON_START    = (1U << 3),
  PS2_BUTTON_UP       = (1U << 4),
  PS2_BUTTON_RIGHT    = (1U << 5),
  PS2_BUTTON_DOWN     = (1U << 6),
  PS2_BUTTON_LEFT     = (1U << 7),
  PS2_BUTTON_L2       = (1U << 8),
  PS2_BUTTON_R2       = (1U << 9),
  PS2_BUTTON_L1       = (1U << 10),
  PS2_BUTTON_R1       = (1U << 11),
  PS2_BUTTON_TRIANGLE = (1U << 12),
  PS2_BUTTON_CIRCLE   = (1U << 13),
  PS2_BUTTON_CROSS    = (1U << 14),
  PS2_BUTTON_SQUARE   = (1U << 15)
} Ps2Button;

typedef struct
{
  uint16_t buttons;
  uint8_t right_x;
  uint8_t right_y;
  uint8_t left_x;
  uint8_t left_y;
  uint8_t connected;
  uint8_t analog_mode;
} Ps2State;

void Ps2_Init(void);
void Ps2_ConfigureAnalog(void);
uint8_t Ps2_Poll(Ps2State *state);

#endif
