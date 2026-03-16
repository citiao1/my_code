#include <Arduino.h>



void setup() {
  pinMode(0,OUTPUT);
  Serial.begin(115200);// put your setup code here, to run once:
  
}

void loop() {
  digitalWrite(0,LOW);
  Serial.println("LED ON");
  delay(2000);
  digitalWrite(0,HIGH);
  Serial.println("LED OFF");
  delay(1500);// put your main code here, to run repeatedly:
}

