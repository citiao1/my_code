#include <Arduino.h>
#include<Servo.h>
Servo myservo;
long time1=0;
long servo_time=0;
long key_time=0;
int angle=0;
int servo_state=0;
int servo_flag=0;
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(5)==HIGH){
    servo_state=1;
  }else{
    servo_state=0;
  }
}
void servo_app(){
  time1=millis();
  if(time1-servo_time<20)return;
  servo_time=time1;
  switch (servo_flag)
  {
  case 0:
    angle+=3;
    if(angle>=180)servo_flag=1;
    break;
  case 1:
    angle-=3;
    if(angle<=0)servo_flag=0;
    break;
  default:
    break;
  }
  myservo.write(angle);
  Serial.println(angle);
}
void setup() {
  myservo.attach(4);
  Serial.begin(115200);
  pinMode(5,INPUT);
  myservo.write(angle);
}

void loop() {
  key_pro();
  if(servo_state==1)servo_app();
}


