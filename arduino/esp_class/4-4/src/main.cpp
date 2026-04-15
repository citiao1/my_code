#include <Arduino.h>


void setup() {
  pinMode(5,OUTPUT);
  pinMode(14,INPUT); 
 
}

void loop() {
  if(digitalRead(14)==HIGH)digitalWrite(5,HIGH);
  else digitalWrite(5,LOW);
}

//近LOW远HIGH