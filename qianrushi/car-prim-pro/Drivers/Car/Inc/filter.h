#ifndef FILTER_H
#define FILTER_H

float Filter_Median(float *data, int len);
float Filter_Limit(float new_val, float old_val, float th);
float Filter_LowPass(float new_val, float old_val, float alpha);

#endif