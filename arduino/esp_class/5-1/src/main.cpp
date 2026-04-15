#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
const char* ssid="abc";
const char* password="fsx20060809";
const char* mqtt_server="192.168.28.139";
const char* topic_pub = "temp";
const char* topic_sub = "inTopic";
const char* topic_sub_1="led";
long time1=0;
long temp_time=0;
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
      client.subscribe(topic_sub);
      client.subscribe(topic_sub_1);
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
 
  if(strcmp(topic, topic_sub) == 0){
    Serial.print("云端指令inTopic:");
    Serial.println(msg);
  }else if(strcmp(topic, topic_sub_1) == 0){
    if(msg=="0")digitalWrite(2,LOW);
    else if(msg=="1")digitalWrite(2,HIGH);
    Serial.print("云端指令led:");
    Serial.println(msg);
  }
  
}
void temp(){
  time1=millis();
  if(time1-temp_time<200)return;
  temp_time=time1;
  client.publish(topic_pub,"111");

}
void setup() {
 Serial.begin(115200);
  Serial.println("esp8266准备就绪");
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
  pinMode(2,OUTPUT);
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  temp();
}

