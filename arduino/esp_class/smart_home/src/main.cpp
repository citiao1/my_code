#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
int speed=100;
float temperature=25.0;
float humidity=50.0;
int angle=90;
const char* ssid="abc";
const char* password="fsx20060809";
const char* mqtt_server="10.135.50.139";
const char* topic_pub_light = "fsx_light";
const char* topic_sub_ult = "fsx_ult";
const char* topic_sub_web="fsx_web";
// 初始化 U8G2 (I2C 通信，ESP8266 默认 SDA 为 D2/GPIO4，SCL 为 D1/GPIO5)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
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
    
  }else if(strcmp(topic, topic_sub_web) == 0){
    if(msg=="2ON")digitalWrite(2,LOW);
    else if(msg=="2OFF")digitalWrite(2,HIGH);
    Serial.print("云端指令led:");
    Serial.println(msg);
  }
  
}
void setup() {
  u8g2.begin();
  
  u8g2.enableUTF8Print(); 
}

void loop() {
  u8g2.clearBuffer();  // 清除内部缓冲区
  
  // 【关键步骤 2】设置中文字体
  // wqy12 是 12 像素高度的文泉驿字体，gb2312 包含了绝大多数常用汉字
  u8g2.setFont(u8g2_font_wqy12_t_gb2312); 
  u8g2.drawFrame(0, 0, 128, 64);
  // 设置光标位置 (X坐标, Y坐标)
  // 注意：U8g2 的 Y 坐标是指文字的“底线”(Baseline)，所以不能设置为 0，否则文字会跑到屏幕外面
  u8g2.setCursor(0, 15); 
  u8g2.print("电机:"+String(speed)); // 直接使用 print 打印中文
  u8g2.setCursor(0, 30); 
  u8g2.print("温度:"+String(temperature)+"℃");
  u8g2.setCursor(0, 45); 
  u8g2.print("湿度:"+String(humidity)+"%");  
  u8g2.setCursor(0, 60); 
  u8g2.print("舵机:"+String(angle)+"°"); 
  

  u8g2.sendBuffer();   // 将缓冲区内容发送到屏幕显示
 
}