#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
const char* ssid="CMCC-6Ftg";
const char* password="a9u64egf";
const char* mqtt_server="broker.emqx.io";
const char* topic_pub = "China/Beijing/huayuan/302/status";
const char* topic_sub = "China/Beijing/huayuan/302/command";
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
  Serial.print("云端指令:");
  Serial.println(msg);
}
void setup() {
 Serial.begin(115200);
  Serial.println("esp8266准备就绪");
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
}

