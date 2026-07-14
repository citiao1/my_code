#ifndef _LQ_ENCODER_H
#define _LQ_ENCODER_H

#include "include.h"

extern int LQ_encoder_L, LQ_encoder_R;
extern int LQ_encoder_L_Last, LQ_encoder_R_Last;

void Encoder_Init();

void LQ_Test_Encoder(void);

#endif
