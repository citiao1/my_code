#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include "stdio.h"
const char* ssid="abc";
const char* password="fsx20060809";
const char* mqtt_server="10.126.235.139";
const char* topic_pub_ult = "fsx_ult";
const char* topic_sub_light = "fsx_light";
const char* topic_sub_web="fsx_web";
long time1=0;
const int Trigpin=12;
const int Echopin=13;
int distance=0;
WiFiClient espClient;
PubSubClient client(espClient);
int light=0;
long send_time=0;
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
      client.subscribe(topic_sub_light);
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
 
  if(strcmp(topic, topic_sub_light) == 0){
    sscanf(msg.c_str(),"light:%d",&light);
    if(light>1000)digitalWrite(4,LOW);
    else digitalWrite(4,HIGH);
  }
  else if(strcmp(topic, topic_sub_web) == 0){
    if(msg=="1ON")digitalWrite(0,LOW);
    else if(msg=="1OFF")digitalWrite(0,HIGH);
    Serial.print("云端指令led:");
    Serial.println(msg);
  }
  
}
void ult_pro(){
  digitalWrite(Trigpin,LOW);
  delayMicroseconds(2);
  digitalWrite(Trigpin,HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigpin,LOW);
  distance=pulseIn(Echopin,HIGH);
  distance=distance/58;
}

void send_pro(){
  time1=millis();
  if(time1-send_time<5000)return;
  send_time=time1;
  String msg="distance:"+String(distance);
  client.publish(topic_pub_ult,msg.c_str());
}

void setup() {
 Serial.begin(115200);
  Serial.println("esp8266准备就绪");
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
  pinMode(0,OUTPUT);
  pinMode(Trigpin,OUTPUT);
  pinMode(Echopin,INPUT);
  pinMode(4,OUTPUT);
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  ult_pro();
  send_pro();
}

