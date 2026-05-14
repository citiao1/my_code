#include <Arduino.h>
long time1=0;
long key_time=0;
int value=0;
long led_time=0;
int led_state=0;
int upmode=0;
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(4)==HIGH){
    led_state=1;
  }else{
    led_state=0;
    value=0;
    analogWrite(12,0);
    Serial.println(value);
  }
}
void led_app(){
  time1=millis();
  if(time1-led_time<10)return;
  led_time=time1;
  if(upmode==0){
    value++;
    if(value>=255)upmode=1;
  }else{
    value--;
    if(value<=0)upmode=0;
  }
  analogWrite(12,value);
  Serial.println(value);
}
void setup() {
  pinMode(12,OUTPUT);
  Serial.begin(115200);
  pinMode(4,INPUT);
}

void loop() {
  key_pro();
  if(led_state==1)led_app();
}


