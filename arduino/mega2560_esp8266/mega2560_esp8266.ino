long time1=0;
long key_time=0;
int led_state=0;
long led_time=0;
int light=0;
int knod_time=0;
int last_light=0;
int light1=1;
int knod_set=1;
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.println("mega2560准备就绪");// put your setup code here, to run once:
  pinMode(11,OUTPUT);
  pinMode(3,INPUT_PULLUP);
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
  light1=map(light,0,255,0,100);
  if(last_light+1==light1||last_light-1==light1||last_light==light1)return;
  last_light=light1;
  time1=millis();
  if(time1-knod_time<100)return;
  knod_time=time1;
  Serial1.println("light:"+String(light1));
  }else{return;}
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
      else if(command.indexOf(':')!=-1){
        if(extractNumber(command)!=0){
        light1=extractNumber(command);
        Serial1.println("light:"+String(light1));
        light=map(light1,0,100,0,255);
        knod_set=0;
        }else{
          knod_set=1;
        }
      }
      else{
        Serial.println("无效指令");
        Serial1.println("无效指令");
      }
    }
  }
  key_pro();
  knod_pro();
  led_app(led_state,light);
} 
