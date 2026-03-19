#include <Arduino.h>
#include <Servo.h>
#include <DHT.h>
DHT dht(13,DHT11);
Servo myservo;
long serial_time=0;
long time1=0;
long dht_time=0;
long fan_time=0;
long servo_time=0;
long key_time=0;
float temperature=0;
int auto_mode=0;
int fan_speed=0;
int all_state=1;
int Servo_state=0;
int angle=0;
int angle_state=0;
int fan_state=0;
void dht_app(){
  time1=millis();
  if(time1-dht_time<200)return;
  dht_time=time1;
  temperature=dht.readTemperature();
  Serial.println("Temperature: "+String(temperature,2)+"C");
}
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(5)==HIGH){
    auto_mode^=1;
  }else if(digitalRead(12)==HIGH){
    fan_speed++;
    if(fan_speed>=3)fan_speed=0;
  }else if(digitalRead(14)==HIGH){
    Servo_state^=1;
  }else if(digitalRead(15)==HIGH){
    all_state^=1;
  }
}
void fan_app(){
  time1=millis();
  if(time1-fan_time<100)return;
  fan_time=time1;
  if(auto_mode==1){
    if(temperature<26){
    analogWrite(16,0);
    fan_state=0;
  }else if(temperature>=26&&temperature<30){
    analogWrite(16,500);
    fan_state=1;
  }else if(temperature>=30){
    analogWrite(16,1500);
    fan_state=2;
  }
  }else{
    switch (fan_speed)
    {
    case  0:
      analogWrite(16,0);
      fan_state=0;
      break;
    case 1:
      analogWrite(16,500);
      fan_state=1;
      break;
    case 2:
      analogWrite(16,1500);
      fan_state=2;
      break;
    default:
      break;
    }
  }
}
void servo_app(){
  time1=millis();
  if(time1-servo_time<30)return;
  servo_time=time1;
  if(auto_mode==1||(auto_mode==0&&Servo_state==1)){
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
  }
  myservo.write(angle); 

}
void serial_app(){
  time1=millis();
  if(time1-serial_time<200)return;
  serial_time=time1;
  if(auto_mode==1){
    Serial.print("自动模式 ");
  }else{
    Serial.print("手动模式 ");
  }
  switch (fan_state)
  {
  case 0:
    Serial.print("风扇停转 ");
    break;
  case 1:
    Serial.print("风扇低速 ");
    break;
  case 2:
    Serial.print("风扇高速 ");
  default:
    break;
  }
  if(auto_mode==1||(auto_mode==0&&Servo_state==1)){
    Serial.print("开始摇头 ");
  }else{
    Serial.print("停止摇头 ");
  }
  Serial.println("室温:"+String(temperature,2)+"℃ 448");
}
void setup() {
  Serial.begin(115200);
  myservo.attach(4);
  myservo.write(0);
  pinMode(5,INPUT);
  pinMode(15,INPUT);
  pinMode(12,INPUT);
  pinMode(14,INPUT);
  pinMode(A0,INPUT);
  dht.begin();
}

void loop() {
  key_pro();
  dht_app();
  if(all_state){
    fan_app();
    servo_app();
  }else{
    analogWrite(16,0);
    fan_state=0;
    Servo_state=0;
  }
  serial_app();
}


