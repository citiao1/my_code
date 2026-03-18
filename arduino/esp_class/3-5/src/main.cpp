#include <Arduino.h>


long time1=0;
long serial_time=0;
int speed=0;
void serial_out(){
  time1=millis();
  if(time1-serial_time<100)return;
  serial_time=time1;
  Serial.println(speed);
}
void setup() {
  Serial.begin(115200);

}

void loop() {
  speed=analogRead(A0);
  speed=map(speed,0,1023,0,1500);
  serial_out();
  analogWrite(13,speed);
}

