#include "mecanum.h"

static int32_t Mecanum_Abs(int32_t value)
{
  return (value >= 0) ? value : -value;
}

static int16_t Mecanum_Scale(int32_t value, int32_t maximum, uint16_t limit)
{
  if (maximum <= (int32_t)limit)
  {
    return (int16_t)value;
  }
  return (int16_t)((value * (int32_t)limit) / maximum);
}

void Mecanum_CalculateTargets(int16_t forward_rpm, int16_t left_rpm,
                              int16_t rotate_rpm, uint16_t limit_rpm,
                              MecanumWheelTargets *targets)
{
  int32_t wheel[4];
  int32_t maximum;
  uint8_t index;

  if (targets == 0)
  {
    return;
  }

  /*
   * Installed order is [RF, LF, LR, RR], viewed from above.
   * This is the standard mecanum transform reordered to the actual
   * motor channels. Positive rotation is counter-clockwise: right
   * wheels advance and left wheels reverse.
   */
  wheel[0] = (int32_t)forward_rpm + left_rpm + rotate_rpm; /* motor 1 RF */
  wheel[1] = (int32_t)forward_rpm - left_rpm - rotate_rpm; /* motor 2 LF */
  wheel[2] = (int32_t)forward_rpm + left_rpm - rotate_rpm; /* motor 3 LR */
  wheel[3] = (int32_t)forward_rpm - left_rpm + rotate_rpm; /* motor 4 RR */

  maximum = 0;
  for (index = 0U; index < 4U; ++index)
  {
    if (Mecanum_Abs(wheel[index]) > maximum)
    {
      maximum = Mecanum_Abs(wheel[index]);
    }
  }

  targets->motor1_right_front = Mecanum_Scale(wheel[0], maximum, limit_rpm);
  targets->motor2_left_front = Mecanum_Scale(wheel[1], maximum, limit_rpm);
  targets->motor3_left_rear = Mecanum_Scale(wheel[2], maximum, limit_rpm);
  targets->motor4_right_rear = Mecanum_Scale(wheel[3], maximum, limit_rpm);
}
