#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
const char* ssid="abc";
const char* password="fsx20060809";
const char* mqtt_server="10.135.50.139";
const char* topic_pub_light = "fsx_light";
const char* topic_sub_ult = "fsx_ult";
const char* topic_sub_web="fsx_web";
long time1=0;
int distance=0;
long lcd_time=0;
long send_time=0;
long light_time=0;
int light=0;
WiFiClient espClient;
PubSubClient client(espClient);
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("正在连接:");
  Serial.println(ssid);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n连接成功");
}
void reconnect(){
  while(!client.connected()){
    Serial.print("正在连接MQTT...");
    String clientId="ESP8266-"+String(random(0xffff),HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("连接成功");
      client.subscribe(topic_sub_ult);
      client.subscribe(topic_sub_web);
    }else{
      Serial.print("连接失败,rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}
void callback(char* topic,byte* payload,unsigned int length){
  String msg="";
  for(int i=0;i<length;i++){
    msg+=char(payload[i]);
  }
 
  if(strcmp(topic, topic_sub_ult) == 0){
    sscanf(msg.c_str(),"distance:%d",&distance);
    lcd.setCursor(0,0);
    lcd.print("The current distance is ");
    lcd.setCursor(0,1);
    lcd.print(String(distance)+" cm                           ");
  }else if(strcmp(topic, topic_sub_web) == 0){
    if(msg=="2ON")digitalWrite(2,LOW);
    else if(msg=="2OFF")digitalWrite(2,HIGH);
    Serial.print("云端指令led:");
    Serial.println(msg);
  }
  
}
void light_pro(){
  time1=millis();
  if(time1-light_time<200)return;
  light_time=time1;
  light=analogRead(A0);
}
void send_pro(){
  time1=millis();
  if(time1-send_time<2000)return;
  send_time=time1;
  String msg="light:"+String(light);
  client.publish(topic_pub_light,msg.c_str());
}
void setup() {
 Serial.begin(115200);
  Serial.println("esp8266准备就绪");
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
  pinMode(2,OUTPUT);
  lcd.init();
  lcd.backlight();
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  light_pro();
  send_pro();
}

