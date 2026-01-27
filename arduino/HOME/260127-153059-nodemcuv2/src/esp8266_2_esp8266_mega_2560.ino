#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
MFRC522 mfrc522(15,0);
const char* ssid="CMCC-6Ftg";
const char* password="a9u64egf";
const char* mqtt_server="broker.emqx.io";
const char* topic_pub = "China/Beijing/huayuan/302/status";
const char* topic_sub = "China/Beijing/huayuan/302/command";
const char* topic_pub_door = "China/Beijing/huayuan/302/door/status";
const char* topic_sub_door = "China/Beijing/huayuan/302/door/command";
long time1=0;
long key_time=0;
int led_state=0;
int sound_state=0;
long sound_time=0;
long led_time=0;
long blink_time=0;
int blink_state=0;
int blink_flag=0;
long blinking_time=0;
long rfid_time=0;
WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;
String content="";
String card=" 92 D2 2A07";
void setup() {
  
  pinMode(16,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(4,INPUT_PULLUP);
  pinMode(10,OUTPUT);
  Serial.begin(115200);
  Serial.println("esp8266_1准备就绪");
  setup_wifi();
  SPI.begin();
  mfrc522.PCD_Init();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback); 
  myServo.attach(2);
  myServo.write(0);
}

void setup_wifi(){
  delay(10);
  Serial.println("正在连接"+String(ssid));
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n连接成功");
}
void reconnect(){
  while(!client.connected()){
    Serial.println("正在连接MQTT");
    String clientId="ESP8266-"+String(random(0xffff),HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("连接成功");
      client.subscribe(topic_sub_door);
    }
    else{
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
  if(msg=="guest"){
    blink_flag=1;
    time1=millis();
    blink_time=time1;
  }else if(msg=="door open"){
    blink_flag=0;
    myServo.write(180);
  }else if(msg=="door close"){
    myServo.write(0);
  }
}
void key_pro(){
  time1=millis();
  if(time1-key_time<200)return;
  key_time=time1;
  if(digitalRead(4)==0){
      led_state=1;
      sound_state=1;
      time1=millis();
      led_time=time1;
      sound_time=time1;
      client.publish(topic_pub_door,"客人来了");
  }
}
void sound_app(){
  time1=millis();
  while(time1-sound_time<600){
    tone(5,784);
    return;
  }
  while(time1-sound_time<1600){
    tone(5,659);
    return;
  }noTone(5);
  sound_state=0;

}

void led_app(){
  time1=millis();
  if(time1-led_time<5000){
    digitalWrite(10, HIGH);
    return;
  }digitalWrite(10,LOW);
  led_state=0; 
}
void blink(int blink_time1){
  time1=millis();
  if(time1-blink_time<30000){
      if(time1-blinking_time<blink_time1)return;
      blinking_time=time1;
      blink_state^=1;
      if(blink_state==1){
        digitalWrite(16, HIGH);
      }else{
        digitalWrite(16, LOW);
      }
      return;
  }blink_flag=0;
}

void rfid_pro(){
  time1=millis();
  if(time1-rfid_time<200)return;
  rfid_time=time1;
  if ( ! mfrc522.PICC_IsNewCardPresent())return;
  if ( ! mfrc522.PICC_ReadCardSerial())return;
  for(byte i=0;i<mfrc522.uid.size;i++){
    
    content.concat(String(mfrc522.uid.uidByte[i]<0x10? "0":" "));
    content.concat(String(mfrc522.uid.uidByte[i],HEX));
  }

  content.toUpperCase();

  if(content==card){
    myServo.write(180);
    client.publish(topic_pub_door,"欢迎回家");
  }else{
    sound_state=1;
    myServo.write(0);
    time1=millis();
    sound_time=time1;
    client.publish(topic_pub_door,"错误门卡");
  }
  content="";
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  key_pro();
  rfid_pro();
  if(sound_state==1){
    sound_app();
  }
  if(led_state==1){
    led_app();
  }
  if(blink_flag==1){
    blink(200);
  }else{
    digitalWrite(16, LOW);
 
  }
}


