#include <Arduino.h>

long time1=0;
long key_time=0;


void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(5)==HIGH){
    digitalWrite(12,HIGH);
    Serial.println("红色");
  }else{
    digitalWrite(12,LOW);
  }
  if(digitalRead(13)==HIGH){
    digitalWrite(14,HIGH);
    Serial.println("黄色");
  }else{
    digitalWrite(14,LOW);
  }
  if(digitalRead(4)==HIGH){
    digitalWrite(16,HIGH);
    Serial.println("绿色");
  }else{
    digitalWrite(16,LOW);
  }

}

void setup() {
  Serial.begin(115200);
  Serial.println("傅思雄 2024030448");
  pinMode(12,OUTPUT);
  pinMode(14,OUTPUT);
  pinMode(16,OUTPUT);
  pinMode(5,INPUT);
  pinMode(13,INPUT);
  pinMode(4,INPUT);
}

void loop() {
  key_pro();

}