#include <Arduino.h>
#include "Servo.h"
// put function declarations here:
const int Trigpin=12;
const int Echopin=13;
long time1=0;
int distance=0;
int ult_flg=0;
int hongwai_flag=0;
Servo myservo;
long servo_time;
int angle=0;
int angle_state=0;
long buzzer_time=0;
int fre=1000;
int buzzer_state=0;
long serial_time=0;
void buzzer_app(int time2){
  time1=millis();
  if(time1-buzzer_time<time2)return;
  buzzer_time=time1;
  buzzer_state^=1;
  if(buzzer_state)tone(15,fre);
  else noTone(15);
}
void hongwai(){
  if(digitalRead(14)==HIGH)hongwai_flag=0;
  else hongwai_flag=1;
}


void ult_pro(){
  digitalWrite(Trigpin,LOW);
  delayMicroseconds(2);
  digitalWrite(Trigpin,HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigpin,LOW);
  distance=pulseIn(Echopin,HIGH);
  distance=distance/58;
  if(distance<50)ult_flg=1;
  else ult_flg=0;
}
void servo_app(){
  time1=millis();
  if(time1-servo_time<30)return;
  servo_time=time1;
    switch (angle_state)
    {
    case 0:
      angle+=3;
      if(angle>=180){
        angle=180;
        angle_state=1;
      }
      break;
    case 1:
      angle-=3;
      if(angle<=0){
        angle=0;
        angle_state=0;
      }
    default:
      break;
    }
  
  myservo.write(angle); 

}
void serial_app(){
  time1=millis();
  if(time1-serial_time<200)return;
  serial_time=time1;
  Serial.println("ult:"+String(distance)+"hongwai:"+String(hongwai_flag));
}
void setup() {
  Serial.begin(115200);
  myservo.attach(4);
  myservo.write(0);
  pinMode(14,INPUT); // put your setup code here, to run once:
  pinMode(Trigpin,OUTPUT);
  pinMode(Echopin,INPUT);
}

void loop() {
  hongwai();
  ult_pro();
  serial_app();
  if(hongwai_flag==0&&ult_flg==0){
    buzzer_app(1000);
    servo_app();
  }else if(hongwai_flag==0&&ult_flg==1){
    buzzer_app(500);
    servo_app();
  }else if(hongwai_flag==1&&ult_flg==1){
    buzzer_app(250);
    myservo.write(0);
  }
}
