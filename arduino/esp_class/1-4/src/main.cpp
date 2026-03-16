#include <Arduino.h>

long time1=0;
long key_time=0;
int led_flag=0;
int led_state=0;
long blink_time=0;
long led_time=0;
int blink_flag=0;
int out_flag=0;
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(5)==HIGH){
    led_flag=1;
    while(digitalRead(5)==HIGH);
  }
  
  if(digitalRead(4)==HIGH){
    Serial.println("停止工作");
    led_flag=0;
    digitalWrite(12,LOW);
      digitalWrite(14,LOW);
      digitalWrite(16,LOW);
    while(digitalRead(4)==HIGH);
 
    
  }

}
void blink(int time){
  time1=millis();
  if(time1-blink_time<time)return;
  blink_time=time1;
  blink_flag^=1;
  if(blink_flag){
    digitalWrite(14,HIGH);
  }else{
    digitalWrite(14,LOW);
    
  }
}
void led_app(){
  time1=millis();
  if(time1-led_time>3000){
  led_state++;
  led_time=time1;
  out_flag=0;}
  if(led_state>2)led_state=0;
  switch(led_state){
    case 0:
      digitalWrite(12,HIGH);
      digitalWrite(14,LOW);
      digitalWrite(16,LOW);
      if(out_flag==0){
        Serial.println("红灯亮");
        out_flag=1;}
      break;
    case 1:
      digitalWrite(12,LOW);
      blink(200);
      digitalWrite(16,LOW);
      
      if(out_flag==0){
        Serial.println("黄灯闪");
        out_flag=1;}
      break;
    case 2:
      digitalWrite(12,LOW);
      digitalWrite(14,LOW);
      digitalWrite(16,HIGH);
      
      if(out_flag==0){
        Serial.println("绿灯亮");
        out_flag=1;}
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(12,OUTPUT);
  pinMode(14,OUTPUT);
  pinMode(16,OUTPUT);
  pinMode(5,INPUT);
  pinMode(13,INPUT);
  pinMode(4,INPUT);
}

void loop() {
  key_pro();
  if(led_flag==1)led_app();
}