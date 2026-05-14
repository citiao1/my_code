#include <Arduino.h>
long time1=0;
long key_time=0;
int fan_flag=0;
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(5)==HIGH){
    fan_flag^=1;
    if(fan_flag==1){
      Serial.println("启动");
    }else{
      Serial.println("关闭");
    }
    while(digitalRead(5)==HIGH);
  }
}


void setup() {
  pinMode(5,INPUT);
 
  Serial.begin(115200);

}

void loop() {
  key_pro();
  if(fan_flag==1){
    analogWrite(16,500);
  }else{
    analogWrite(16,0);
  }
}
// put function definitions here:
