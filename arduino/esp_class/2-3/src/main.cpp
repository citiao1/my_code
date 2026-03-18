#include <Arduino.h>
long time1=0;
long light_time=0;
int light=0;
void light_pro(){
  time1=millis();
  if(time1-light_time<100)return;
  light_time=time1;
  light=analogRead(A0);
  Serial.println(light);
}

void setup() {
  Serial.begin(115200);
  pinMode(12,OUTPUT);
  pinMode(A0,INPUT);

}

void loop() {
  light_pro();
  if(light>150){
    digitalWrite(12,LOW);
  }else{
    digitalWrite(12,HIGH);
  }
}

