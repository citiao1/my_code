#include <Arduino.h>
#include <Servo.h>
Servo myservo;
void setup() {
  myservo.attach(4);
  myservo.write(60);
  Serial.begin(115200);

}

void loop() {
  if(analogRead(A0)>15){
    myservo.write(60);
    delay(100);
    myservo.write(120);
    delay(100);
  }else{
    myservo.write(0);
  }
}


