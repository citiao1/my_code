#include <Arduino.h>

// put function declarations here:
long time1=0;
long key_time=0;


void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(4)==HIGH){
    digitalWrite(2,LOW);
    Serial.println("Button Pressed,led on");
  }
  else{
    digitalWrite(2,HIGH);
    Serial.println("Button Released,led off");
  }

}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(2,OUTPUT);
  pinMode(4,INPUT);

}

void loop() {
  key_pro();
}

// put function definitions here:
