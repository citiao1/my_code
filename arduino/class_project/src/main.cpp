#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
const char* ssid="abc";
const char* password="fsx20060809";
const char* mqtt_server="broker.emqx.io";
const char* topic_pub="Beijing/F205/status";
const char* topic_sub="Beijing/F205/command";
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi(){
  delay(10);
  Serial.print("正在连接"+String(ssid));
  WiFi.begin(ssid,password);
  while (WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

  }Serial.println("\n连接成功");
  
}
void reconnect(){
  while(!client.connected()){
    Serial.println("正在连接MQTT");
    String clientId="ESP8266-"+String(random(0xffff));
    if(client.connect(clientId.c_str())){
      Serial.println("连接成功");
      client.subscribe(topic_sub);
    }else{
      Serial.println("连接失败,rc="+String(client.state()));
      delay(500);
    }
  }
}
void callback(char* topic,byte* payload,unsigned int length){
  String msg="";
  for(int i=0;i<length;i++){
    msg+=char(payload[i]);
  }
  Serial.println("云端命令:"+msg);
  if(msg=="ON"){
    Serial.println("LED ON");
    digitalWrite(2,LOW);
  }
  if(msg=="OFF"){
    Serial.println("LED OFF");
    digitalWrite(2,HIGH);
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println("esp8266 is ready");
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
  
}

