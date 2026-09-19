#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#define ENCODER_CHANNEL_COUNT 4U

typedef struct
{
  int32_t total[ENCODER_CHANNEL_COUNT];
  int32_t delta[ENCODER_CHANNEL_COUNT];
  int32_t rpm_x10[ENCODER_CHANNEL_COUNT];
  uint16_t counts_per_revolution;
  uint8_t ready;
} EncoderSnapshot;

uint8_t Encoder_Init(void);
void Encoder_Process(uint32_t elapsed_ms);
void Encoder_Zero(void);
uint8_t Encoder_SetCountsPerRevolution(uint16_t counts_per_revolution);
EncoderSnapshot Encoder_GetSnapshot(void);
void Encoder_ExtiCallback(uint16_t gpio_pin);

#endif /* ENCODER_H */
