#ifndef __INDUCTANCE_H_
#define __INDUCTANCE_H_

#include "AI8051U_ADC.h"
#include "string.h"
#include <stdio.h>
#include "include.h"
#include "LQ_ADC.h"

void ScanInductance(void);
int* GetInductance(void);
extern int L1, L2, M, R1, R2;
#endif