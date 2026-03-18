#include <Arduino.h>
#include <dht.h>
long time1=0;
long dht_time=0;
float temperature=0;
long fan_time=0;
long key_time=0;
int fan_state=1;
DHT dht(13,DHT11);
void dht_app(){
  time1=millis();
  if(time1-dht_time<200)return;
  dht_time=time1;
  temperature=dht.readTemperature();
  Serial.println("Temperature: "+String(temperature,2)+"C");
}
void fan_app(){
  time1=millis();
  if(time1-fan_time<100)return;
  fan_time=time1;
  if(temperature<26){
    analogWrite(16,0);
    Serial.println("风扇停转");
  }else if(temperature>=26&&temperature<30){
    analogWrite(16,500);
    Serial.println("低速旋转");
  }else if(temperature>=30){
    analogWrite(16,1500);
    Serial.println("高速旋转");
  }
}
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(4)==HIGH){
    fan_state^=1;
    if(fan_state==0)Serial.println("风扇停转");
    while(digitalRead(4)==HIGH);
  }
}
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(4,INPUT);
}

void loop() {
  dht_app();
  key_pro();
  if(fan_state==1){fan_app();}
  else{
    analogWrite(16,0);
  }
}

