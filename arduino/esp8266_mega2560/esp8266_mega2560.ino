#include<SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
SoftwareSerial mySerial(14,12);
LiquidCrystal_I2C lcd(0x27,16,2);
const char* ssid="CMCC-6Ftg";
const char* password="a9u64egf";
const char* mqtt_server="broker.emqx.io";
const char* topic_pub = "China/Beijing/huayuan/302/status";
const char* topic_sub = "China/Beijing/huayuan/302/command";
WiFiClient espClient;
PubSubClient client(espClient);
int led_state=0;
int light=0;
int val=0;
void setup() {
  Serial.begin(115200);
  Serial.println("esp8266准备就绪");
  mySerial.begin(9600);
  Wire.begin(4,5);
  lcd.init();
  lcd.backlight();
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);

}
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("正在连接:");
  Serial.println(ssid);
  
  lcd.setCursor(0,0);
  lcd.print("connecting...            ");
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n连接成功");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi OK!");
}
void reconnect(){
  while(!client.connected()){
    Serial.print("正在连接MQTT...");
    lcd.setCursor(0,1);
    lcd.print("MQTT connecting...");
    String clientId="ESP8266-"+String(random(0xffff),HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("连接成功");
      client.subscribe(topic_sub);
      lcd.setCursor(0,1);
      lcd.print("MQTT OK!          ");
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
  mySerial.println(msg);
}
void lcd_pro(int light){
  if(led_state==1){
    
    lcd.setCursor(0,0);
    lcd.print("LED STATE:ON                ");
  }else{
    
    lcd.setCursor(0,0);
    lcd.print("LED STATE:OFF                 ");
  }
  lcd.setCursor(0,1);
  lcd.print("LIGHT:"+String(light)+"%                      ");
}
void loop() {
  if(!client.connected()){
    reconnect();
  }
    client.loop();
  if(mySerial.available()){
    String data=mySerial.readStringUntil('\n');
    data.trim();
    if(data.length()>0&&data[0]>127){
      Serial.print("Mega:");
      Serial.println(data);
      client.publish(topic_pub,data.c_str());
    }else if(data.length()>0&&data[0]<=127){
      if(data=="LED ON"){
        led_state=1;
        Serial.println("lcd:led on");
      }else if(data=="LED OFF"){
        led_state=0;
        Serial.println("lcd:led off");
      }else if(data[0]=='l'){
        int index=data.indexOf(':');
        String Num=data.substring(index+1);
        val=Num.toInt();
      }
    
    }
    lcd_pro(val);
  } 
  
// put your main code here, to run repeatedly:

}
