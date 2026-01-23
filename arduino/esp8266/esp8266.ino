#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<LiquidCrystal_I2C.h>
#include<Wire.h>
const char* ssid="CMCC-6Ftg";
const char* password="a9u64egf";
const char* mqtt_server="broker.emqx.io";
LiquidCrystal_I2C lcd(0x27,16,2);
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi(){
  delay(10);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n网络连接成功！");
  lcd.setCursor(0,0);
  lcd.print("WiFi OK!       ");
}


void callback(char* topic,byte* payload,unsigned int length){
  String msg="";
  for(int i=0;i<length;i++){
    msg+=(char)payload[i];
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Msg:");
  lcd.setCursor(0,1);
  lcd.print(msg);

  
}
void reconnect(){
  while(!client.connected()){
    Serial.print("正在连接服务器...");
    String clientID="ESP8266-"+String(random(0xffff),HEX);
    if(client.connect(clientID.c_str())){
      Serial.println("MQTT连接成功！");
      lcd.setCursor(0, 1);
      lcd.print("MQTT Connected");
      client.subscribe("China/Beijing/huayuan/302/message");

    }else{
      Serial.print("MQTT连接失败,rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup(){
  Serial.begin(115200);
  Wire.begin(4,5);
  lcd.init();
  lcd.backlight();
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
}


void loop(){
  if(!client.connected()){
    reconnect();
  }
  client.loop();
}