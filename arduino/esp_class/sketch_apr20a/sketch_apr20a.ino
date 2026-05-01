#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <DHT.h>

// ================= 1. WiFi和MQTT配置 =================
const char* ssid = "abc";                           // WiFi 名称
const char* password = "fsx20060809";               // WiFi 密码
const char* mqtt_server = "10.185.92.139";         // MQTT 服务器地址
const char* topic_pub_command = "fsx_command";      // MQTT 发布命令主题
const char* topic_sub_dht = "fsx_dht";              // MQTT 订阅温湿度主题
const char* topic_sub_state = "fsx_state";          // MQTT 订阅状态主题
const char* topic_sub_web = "fsx_web";              // MQTT 订阅网页控制主题

// ================= 2. 硬件引脚定义 =================
// 依据 NodeMCU/Wemos D1 常见引脚映射
#define LED_PIN 5       // GPIO5 (D1) - LED
#define FAN_PIN 16       // GPIO4 (D2) - 风扇(可通过继电器或MOS管)
#define RELAY_PIN 14    // GPIO14 (D5) - 空调继电器
#define SERVO_PIN 12    // GPIO12 (D6) - 窗帘舵机
#define DHT_PIN 2       // GPIO2 (D4) - 温湿度传感器数据引脚

#define DHTTYPE DHT11   // 如果使用的是DHT22请修改为 n

// ================= 3. 全局对象与状态变量 =================
WiFiClient espClient;
PubSubClient client(espClient);
Servo curtainServo;
DHT dht(DHT_PIN, DHTTYPE);

// 设备状态记录 (0:关, 1:开)
int fan_state = 0;
int led_state = 0;
int servo_state = 0;
int condition_state = 0;

// ================= 4. 功能函数 =================

// 初始化WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("正在连接WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi连接成功!");
}

// 重新连接MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("正在连接MQTT...");
    String clientId = "ESP8266-Device2-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("连接成功!");
      // 核心：板子2必须订阅指令主题
      client.subscribe(topic_pub_command); 
    } else {
      Serial.print("失败, rc=");
      Serial.print(client.state());
      Serial.println(" 5秒后重试");
      delay(5000);
    }
  }
}

// 上传当前设备状态
void publish_state() {
  // 按照主板要求的格式：state:风扇,LED,窗帘,空调
  String state_msg = "state:" + String(fan_state) + "," + 
                     String(led_state) + "," + 
                     String(servo_state) + "," + 
                     String(condition_state);
  client.publish(topic_sub_state, state_msg.c_str());
  Serial.println("已同步状态: " + state_msg);
}

// 读取并上传温湿度
void publish_dht() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("读取温湿度失败!");
    return;
  }
  
  // 按照主板要求的格式：dht:温度,湿度
  String dht_msg = "dht:" + String(t, 1) + "," + String(h, 1);
  client.publish(topic_sub_dht, dht_msg.c_str());
  Serial.println("已上传温湿度: " + dht_msg);
}

// 执行所有设备开/关操作
void control_all(bool turnOn) {
  if (turnOn) {
    digitalWrite(LED_PIN, HIGH);      led_state = 1;
    digitalWrite(FAN_PIN, HIGH);      fan_state = 1;
    digitalWrite(RELAY_PIN, HIGH);    condition_state = 1;
    curtainServo.write(180);          servo_state = 1; // 舵机正转打开
  } else {
    digitalWrite(LED_PIN, LOW);       led_state = 0;
    digitalWrite(FAN_PIN, LOW);       fan_state = 0;
    digitalWrite(RELAY_PIN, LOW);     condition_state = 0;
    curtainServo.write(0);            servo_state = 0; // 舵机反转关闭
  }
}

// MQTT接收回调函数 (解析ESP1发来的指令)
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim(); // 去除多余空格和换行
  Serial.println("收到云端指令: " + msg);

  bool state_changed = false; // 标记状态是否发生改变

  // 解析具体指令
  if (msg == "led_on") {
    digitalWrite(LED_PIN, HIGH); led_state = 1; state_changed = true;
  } else if (msg == "led_off") {
    digitalWrite(LED_PIN, LOW); led_state = 0; state_changed = true;
  } else if (msg == "fan_on") {
    digitalWrite(FAN_PIN, HIGH); fan_state = 1; state_changed = true;
  } else if (msg == "fan_off") {
    digitalWrite(FAN_PIN, LOW); fan_state = 0; state_changed = true;
  } else if (msg == "servo_on") {
    curtainServo.write(180); servo_state = 1; state_changed = true; // 模拟打开窗帘
  } else if (msg == "servo_off") {
    curtainServo.write(0); servo_state = 0; state_changed = true;   // 模拟关闭窗帘
  } else if (msg == "condition_on") {
    digitalWrite(RELAY_PIN, HIGH); condition_state = 1; state_changed = true;
  } else if (msg == "condition_off") {
    digitalWrite(RELAY_PIN, LOW); condition_state = 0; state_changed = true;
  } else if (msg == "all_on") {
    control_all(true); state_changed = true;
  } else if (msg == "all_off") {
    control_all(false); state_changed = true;
  } 
  // 解析状态查询指令
  else if (msg == "get_state") {
    publish_state(); // 按键2请求获取状态
  } else if (msg == "get_dht") {
    publish_dht();   // 按键1请求获取温湿度
  }

  // 如果执行了设备操作，自动回传最新状态
  if (state_changed) {
    publish_state();
  }
}

// ================= 5. 主程序 =================
void setup() {
  Serial.begin(115200);
  
  // 初始化引脚模式
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  // 初始化设备为关闭状态
  digitalWrite(LED_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  
  // 初始化舵机和DHT
  curtainServo.attach(SERVO_PIN,500,2500);
  curtainServo.write(0); // 默认窗帘关闭
  dht.begin();

  // 启动网络与MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // 维持MQTT心跳并处理回调
}