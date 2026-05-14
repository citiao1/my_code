#include <Arduino.h>

long time1=0;
long knob_time=0;
int value = 0;
void knob_pro(){
  time1=millis();
  if(time1-knob_time<20)return;
  knob_time=time1;
  value=analogRead(A0);
  Serial.println(value);
  
}
void setup() {
  Serial.begin(115200);
    
  
}

void loop() {
    knob_pro();
}


