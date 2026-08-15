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

  /* Wheel order: right rear, left rear, right front, left front. */
  /* Positive rotate_rpm means left rotation: right wheels forward. */
  wheel[0] = (int32_t)forward_rpm - left_rpm + rotate_rpm;
  wheel[1] = (int32_t)forward_rpm + left_rpm - rotate_rpm;
  wheel[2] = (int32_t)forward_rpm + left_rpm + rotate_rpm;
  wheel[3] = (int32_t)forward_rpm - left_rpm - rotate_rpm;

  maximum = 0;
  for (index = 0U; index < 4U; ++index)
  {
    if (Mecanum_Abs(wheel[index]) > maximum)
    {
      maximum = Mecanum_Abs(wheel[index]);
    }
  }

  targets->right_rear = Mecanum_Scale(wheel[0], maximum, limit_rpm);
  targets->left_rear = Mecanum_Scale(wheel[1], maximum, limit_rpm);
  targets->right_front = Mecanum_Scale(wheel[2], maximum, limit_rpm);
  targets->left_front = Mecanum_Scale(wheel[3], maximum, limit_rpm);
}
