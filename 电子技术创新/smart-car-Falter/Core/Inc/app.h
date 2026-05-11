#ifndef APP_H
#define APP_H
extern int left_speed;
extern int right_speed;
extern float kp;
extern char state[30];
void App_Init(void);
void App_Update(float dist_1,float dist_2,float dist_3);
#endif