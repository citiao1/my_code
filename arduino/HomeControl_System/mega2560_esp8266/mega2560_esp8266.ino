#include<DHT.h>
long time1=0;
long key_time=0;
int led_state=0;
long led_time=0;
int light=0;
int knod_time=0;
int last_light=0;
int light1=1;
int knod_set=1;
int guest_flag=0;
int door_state=0;
long sound_time=0;
int sound_state=0;
int sound_flag=0;
long home_time=0;
DHT dht(13,DHT11);
struct home_status{
  float humidity=0;
  float temperature=0;
  int light;
  int door_state;
  int light_state;
}home_status;
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.println("mega2560准备就绪");
  dht.begin();
  pinMode(11,OUTPUT);
  pinMode(8,OUTPUT);
  pinMode(3,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  pinMode(A0,INPUT);
  analogWrite(11,0);
}
int extractNumber(String str) {
  String numStr = "";
  for (int i = 0; i < str.length(); i++) {
    if (isDigit(str[i])) { 
      numStr += str[i];
    }
  }
  if (numStr.length() > 0) {
    return numStr.toInt();
  } else {
    return 0;
  }
}
void key_pro(){
  time1=millis();
  if(time1-key_time<200)return;
  key_time=time1;
  if(digitalRead(3)==0){
      led_state^=1;
  if(led_state==1){
    Serial1.println("LED ON");
  }else{
    Serial1.println("LED OFF");
  }
  }
  if(digitalRead(7)==0){
    guest_flag=0;
    door_state^=1;
    
    while(digitalRead(7)==0);
    if(door_state==1){
      Serial1.println("door open");
      
    }else{
      Serial1.println("door close");
    }
  }
}
void led_app(int led_state,int light){
  
  if(led_state==1){
    analogWrite(11,light);
  }else{
    analogWrite(11,0);
  }
}
void knod_pro(){
  if(knod_set){
  light=analogRead(A0);
  light=map(light,0,670,0,255);
  if(light>255){
    light=255;
  }
  light1=map(light,0,255,0,100);
  if(light1>100){
    light1=100;
  }
  if(last_light+1==light1||last_light-1==light1||last_light==light1)return;
  last_light=light1;
  time1=millis();
  if(time1-knod_time<100)return;
  knod_time=time1;
  Serial1.println("light:"+String(light1));
  }else{return;}
}

void sound_app(int sounding_time){
    time1=millis();
    if(sound_flag==1){
      if(time1-sound_time<sounding_time)return;
      sound_time=time1;
      sound_state^=1;
      if(sound_state==1){
        digitalWrite(8, HIGH);
      }else{
        digitalWrite(8, LOW);
      }
    }
    else{
      digitalWrite(8, LOW);
      return;
    }

}
void home_app(){
  time1=millis();
  if(time1-home_time<200)return;
  home_time=time1;
  home_status.humidity=dht.readHumidity();
  home_status.temperature=dht.readTemperature();
  home_status.light_state=led_state;
  home_status.light=light1;
  home_status.door_state=door_state;
  
}
void loop() {
  
  if(Serial1.available()){
    String command=Serial1.readStringUntil('\n');
    command.trim();
    if(command.length()>0){
      Serial.print("收到指令:");
      Serial.println(command);
      if(command=="ON"){
        led_state=1;
        
        Serial.println("执行：开灯");
        Serial1.println("灯光状态：开启");
        Serial1.println("LED ON");

      }else if(command=="OFF"){
        led_state=0;
        
        Serial.println("执行：关灯");
        Serial1.println("灯光状态：关闭");
        Serial1.println("LED OFF");

      }
      else if(command=="light"){
        Serial.println("执行：输出亮度");
        Serial1.println("亮度:"+String(light1)+"%");
      }
      else if(command.indexOf(':')!=-1&&command[0]!='d'){
        if(extractNumber(command)!=0){
        light1=extractNumber(command);
        Serial1.println("light:"+String(light1));
        light=map(light1,0,100,0,255);
        if(light>255){
          light=255;
        }
        knod_set=0;
        }else{
          knod_set=1;
        }
      }
      else if(command=="door:客人来了"){
        Serial1.println("guest");
        guest_flag=1;
        sound_flag=1;
      }
      else if(command=="door open"){
        door_state=1;
        sound_flag=0;
        //Serial1.print("door open");
      }
      else if(command=="door close"){
        door_state=0;
        sound_flag=0;
        //Serial1.print("door close");
      }
      else if(command=="door:door open"){
        door_state=1;
        sound_flag=0;
        Serial1.println("door open");
       
      }
      else if(command=="door:door close"){
        door_state=0;
        sound_flag=0;
        Serial1.println("door close");
        
      }
      else if(command=="get_status"){
        Serial1.println("home_status:"+String(home_status.humidity)+","+String(home_status.temperature)+","+String(home_status.light_state)+","+String(home_status.light)+","+String(home_status.door_state));
        
      }
      else{
        Serial.println("无效指令");
        Serial1.println("无效指令");
      }
    }
  }
  key_pro();
  knod_pro();
  sound_app(200);
  home_app();
  led_app(led_state,light);
} 
