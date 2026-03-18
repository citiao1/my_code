#include <Arduino.h>
#include <Servo.h>
// put function declarations here:
Servo myservo;
int angle=0;

void setup() {
  myservo.attach(4);
  myservo.write(0);
  Serial.begin(115200);
  pinMode(A0,INPUT);
}

void loop() {
  angle=analogRead(A0);
  angle=map(angle,0,1023,0,180);
  myservo.write(angle);
  Serial.println(angle);
  delay(50);
}


