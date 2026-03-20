#include <Arduino.h>
const int Trigpin=12;
const int Echopin=13;
long time1=0;
long buzzer_time=0;
int distance=0;
long serial_time=0;
void ult_pro(){
  digitalWrite(Trigpin,LOW);
  delayMicroseconds(2);
  digitalWrite(Trigpin,HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigpin,LOW);
  distance=pulseIn(Echopin,HIGH);
  distance=distance/58;
}
void buzzer_app(){
  time1=millis();
  if(time1-buzzer_time<200)return;
  buzzer_time=time1;
  tone(15,distance);
}
void serial_app (){
  time1=millis();
  if(time1-serial_time<200)return;
  serial_time=time1;
  Serial.println(distance);
}
void setup() {
  pinMode(Trigpin,OUTPUT);
  pinMode(Echopin,INPUT);
  pinMode(15,OUTPUT);
  Serial.begin(115200);
}

void loop() {
  ult_pro();
  buzzer_app();
  serial_app();
}


