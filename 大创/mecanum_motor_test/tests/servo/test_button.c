#include <assert.h>
#include <stdio.h>
#include "servo_test.h"
static uint32_t now;
static int key, starts, stops, attempts, moving, fail, fail_step, in_sample;
static unsigned process_period=1U;
static int16_t angles[128];
static uint16_t last_speed;
static int16_t feedback_speed;
uint32_t HAL_GetTick(void){return now;}
int HAL_GPIO_ReadPin(int p,int n){(void)p;(void)n;return !key;}
uint8_t St3215_Init(void){assert(!in_sample);return !fail;}
uint8_t St3215_PrepareStep(void){assert(!in_sample);return !fail;}
uint8_t St3215_Start(void){assert(!in_sample);++starts;return !fail;}
uint8_t St3215_Stop(void){assert(!in_sample);++stops;return !fail;}
uint8_t St3215_GetMoving(uint8_t *v){assert(!in_sample);*v=(uint8_t)moving;return !fail;}
uint8_t St3215_GetSpeed(int16_t *v){assert(!in_sample);*v=feedback_speed;return !fail;}
uint8_t St3215_MoveAngle(int16_t a,uint16_t speed){assert(!in_sample && attempts<128);angles[attempts++]=a;last_speed=speed;return !fail&&!fail_step;}
uint8_t St3215_RunSpeed(int8_t d,uint16_t speed){assert(!in_sample);angles[0]=d;last_speed=speed;feedback_speed=(int16_t)(d*speed);return !fail;}
static void advance(unsigned ms){while(ms--){++now;in_sample=1;ServoTest_Sample1ms();in_sample=0;if(process_period&&now%process_period==0)ServoTest_Process();}}
static void reset(int held){key=held;starts=stops=attempts=moving=fail=fail_step=0;process_period=1;ServoTest_Init();}
static void click(void){key=1;advance(20);key=0;advance(20);}
int main(void)
{
  reset(0);advance(2000);assert(attempts==0&&starts==0&&ServoTest_GetStatus()=='S');
  assert(ServoTest_Move(450,600));advance(2);assert(attempts==1&&angles[0]==450&&last_speed==600);
  advance(1800);assert(attempts==1&&ServoTest_GetStatus()=='S');
  assert(ServoTest_Move(-300,500));advance(2);assert(angles[1]==-300);
  assert(ServoTest_Stop());int a=attempts;advance(3000);assert(attempts==a);
  assert(!ServoTest_Move(0,500)&&!ServoTest_Move(7201,500)&&!ServoTest_Move(300,99));
  reset(0);assert(ServoTest_Continuous(-1,800));assert(angles[0]==-1&&last_speed==800);
  advance(2900);assert(ServoTest_GetStatus()=='R');advance(110);assert(ServoTest_GetStatus()=='S'&&stops>0);
  assert(ServoTest_Continuous(1,500));
  for(int i=0;i<8;++i){advance(1000);ServoTest_KeepAlive();}assert(ServoTest_GetStatus()=='R');
  /* Heartbeats and ACKs cannot hide a stopped motor. Faults never restart it. */
  feedback_speed=0;advance(1600);assert(ServoTest_GetStatus()=='E');
  advance(600);assert(ServoTest_GetStatus()=='S');assert(ServoTest_Continuous(-1,500));
  assert(ServoTest_Stop());assert(ServoTest_Continuous(-1,500));advance(300);assert(ServoTest_GetStatus()=='R');
  assert(ServoTest_Stop());assert(ServoTest_Continuous(1,500));advance(300);assert(ServoTest_GetStatus()=='R');
  click();assert(ServoTest_GetStatus()=='S');
  /* Local USER resumes automatic reciprocation without a browser lease. */
  click();advance(1400);assert(attempts>=1&&angles[0]==300);advance(1600);assert(angles[1]==-300);
  key=1;advance(2000);assert(ServoTest_GetStatus()=='S');a=attempts;advance(4000);assert(attempts==a);
  for(unsigned phase=0;phase<80;++phase){reset(0);click();process_period=80;advance(phase);key=1;advance(15);key=0;advance(100);assert(ServoTest_GetStatus()=='S');}
  reset(0);assert(ServoTest_Move(300,500));fail_step=1;advance(2);assert(attempts==1&&ServoTest_GetStatus()=='E');
  fail_step=0;advance(2000);assert(attempts==1&&ServoTest_GetStatus()=='S');
  reset(0);moving=1;click();advance(4000);assert(attempts==1&&ServoTest_GetSnapshot().errors>0);
  now=0xFFFFFF00U;reset(0);assert(ServoTest_Move(-900,1000));advance(2);assert(angles[0]==-900);advance(1800);assert(ServoTest_GetStatus()=='S');
  puts("PASS: standby/custom signed angle/continuous direction/speed/stop/lease/heartbeat/local USER/short presses/no retry/fault/tick-wrap/no ISR UART");
  return 0;
}
