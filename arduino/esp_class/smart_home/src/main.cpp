#include <Arduino.h>          // Arduino 核心库
#include <U8g2lib.h>          // OLED 显示屏库
#include <Wire.h>             // I2C 通信库
#include <ESP8266WiFi.h>      // ESP8266 WiFi 库
#include <PubSubClient.h>     // MQTT 客户端库
#include <string.h>           // 字符串处理库
#include <SoftwareSerial.h>   // 软件串口库



// WiFi 和 MQTT 配置
const char* ssid = "abc";                           // WiFi 名称
const char* password = "fsx20060809";               // WiFi 密码
const char* mqtt_server = "10.185.92.139";         // MQTT 服务器地址
const char* topic_pub_command = "fsx_command";      // MQTT 发布命令主题
const char* topic_sub_dht = "fsx_dht";              // MQTT 订阅温湿度主题
const char* topic_sub_state = "fsx_state";          // MQTT 订阅状态主题
const char* topic_sub_web = "fsx_web";              // MQTT 订阅网页控制主题

// OLED 显示屏初始化
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// WiFi 和 MQTT 客户端
WiFiClient espClient;
PubSubClient client(espClient);

// 软件串口，与模块通信
SoftwareSerial mySerial(14, 12);  // RX=14, TX=12

// 设备控制命令定义
#define LED_ON "00000001"         // LED 开
#define LED_OFF "00000002"        // LED 关
#define FAN_ON "00000003"         // 风扇开
#define FAN_OFF "00000004"        // 风扇关
#define SERVO_ON "00000005"       // 舵机开（窗帘开）
#define SERVO_OFF "00000006"      // 舵机关（窗帘关）
#define CONDITION_ON "00000007"   // 空调开
#define CONDITION_OFF "00000008"  // 空调关
#define ALL_ON "00000009"         // 全部开
#define ALL_OFF "00000010"        // 全部关

// 按键相关变量
int key_value = 0;      // 按键值
long key_time = 0;      // 按键时间戳
long time1 = 0;         // 通用时间戳
long oled_time = 0;     // OLED 显示时间戳
int oled_state = 0;     // OLED 显示状态（0: 显示温湿度, 1: 显示设备状态）
String str = "";        // 字符串缓冲区

// 设备状态结构体
struct state {
  int fan_state;        // 风扇状态 (0: 关, 1: 开)
  int led_state;        // LED 状态 (0: 关, 1: 开)
  int servo_state;      // 舵机状态 (0: 关, 1: 开)
  int condition_state;  // 空调状态 (0: 关, 1: 开)
  float temperature;    // 温度
  float humidity;       // 湿度
} state;

/**
 * @brief WiFi 连接函数
 * @details 连接到指定的 WiFi 网络，等待连接成功并打印状态信息
 */
void setup_wifi(){
  delay(10);  // 短暂延迟，确保系统稳定
  Serial.println();  // 打印空行
  Serial.print("正在连接:");  // 输出连接提示
  Serial.println(ssid);  // 输出 WiFi 名称
  WiFi.begin(ssid,password);  // 开始连接 WiFi
  while(WiFi.status()!=WL_CONNECTED){  // 等待连接成功
    delay(500);  // 每500ms检查一次
    Serial.print(".");  // 输出连接进度点
  }
  Serial.println("\n连接成功");  // 输出WiFi连接成功的信息
}  // 结束WiFi连接函数
/**
 * @brief MQTT 重连函数
 * @details 尝试连接到 MQTT 服务器，如果连接失败则等待 5 秒后重试
 * 连接成功后订阅相关的主题
 */
void reconnect(){
  while(!client.connected()){  // 循环直到MQTT连接成功
    Serial.print("正在连接MQTT...");  // 向串口输出正在连接MQTT的提示
    String clientId="ESP8266-"+String(random(0xffff),HEX);  // 生成随机的客户端ID用于MQTT连接
    if(client.connect(clientId.c_str())){  // 尝试使用生成的客户端ID连接MQTT服务器
      Serial.println("连接成功");  // 输出MQTT连接成功的信息
      client.subscribe(topic_sub_dht);  // 订阅接收温湿度数据的主题
      client.subscribe(topic_sub_state);  // 订阅接收设备状态的主题
      client.subscribe(topic_sub_web);  // 订阅接收网页控制命令的主题
    }else{  // 连接失败
      Serial.print("连接失败,rc=");  // 输出连接失败的提示信息
      Serial.println(client.state());  // 输出MQTT连接失败的错误代码
      delay(5000);  // 延迟5秒钟后进行重试
    }  
  }  
} 
/**
 * @brief MQTT 消息回调函数
 * @param topic 接收到的主题
 * @param payload 消息内容
 * @param length 消息长度
 * @details 处理从 MQTT 服务器接收到的消息，根据主题解析并更新相应的状态
 */
void callback(char* topic,byte* payload,unsigned int length){
  String msg="";  // 初始化消息字符串为空
  for(int i=0;i<length;i++){  // 循环遍历接收到的payload字节数组
    msg+=char(payload[i]);  // 将每个字节转换为字符并拼接到消息字符串
  }  // 结束for循环
 
  if(!strcmp(topic,topic_sub_web)){  // 检查接收的主题是否为网页控制主题
    Serial.println("云端指令:"+msg);  // 如果是网页控制，则输出云端下发的指令到串口
  }else if(!strcmp(topic,topic_sub_dht)){  // 检查接收的主题是否为温湿度主题
    sscanf(msg.c_str(),"dht:%f,%f",&state.temperature,&state.humidity);  // 解析消息格式并提取温度和湿度数据
  }else if(!strcmp(topic,topic_sub_state)){  // 检查接收的主题是否为设备状态主题
    sscanf(msg.c_str(),"state:%d,%d,%d,%d",&state.fan_state,&state.led_state,&state.servo_state,&state.condition_state);  // 解析消息格式并提取各设备状态
  }  // 结束if-else语句
  
}  // 结束MQTT回调函数
/**
 * @brief OLED 显示处理函数
 * @details 根据 oled_state 显示不同的信息：
 * - oled_state=0: 显示温度和湿度
 * - oled_state=1: 显示设备状态（LED、风扇、窗帘、空调）
 * 状态显示 5 秒后自动切换回温湿度显示
 */
void oled_pro(){
  u8g2.clearBuffer();   // 清空显示缓冲区
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 设置中文字体
  if(oled_state==0){  // 如果显示状态为0，显示温湿度
    u8g2.setCursor(0, 30);  // 设置光标位置
    u8g2.print("温度:"+String(state.temperature)+"℃");  // 显示温度
    u8g2.setCursor(0, 45);  // 设置光标位置
    u8g2.print("湿度:"+String(state.humidity)+"%");  // 显示湿度

  }else{  // 否则显示设备状态
  time1=millis();  // 获取当前时间戳
  if(time1-oled_time>5000)oled_state=0;  // 如果显示状态页面超过5秒，切换回温湿度显示
  u8g2.setCursor(0, 15);  // 设置光标位置为第一行
  if(state.led_state==1)u8g2.print("LED:开启");  // 如果LED打开则显示"LED:开启"
  else u8g2.print("LED:关闭");  // 否则显示"LED:关闭"
  u8g2.setCursor(0, 30);  // 设置光标位置为第二行
  if(state.fan_state==1)u8g2.print("风扇:开启");  // 如果风扇打开则显示"风扇:开启"
  else u8g2.print("风扇:关闭");  // 否则显示"风扇:关闭"
  u8g2.setCursor(0, 45);  // 设置光标位置为第三行
  if(state.servo_state==1)u8g2.print("窗帘:开启");  // 如果舵机打开则显示"窗帘:开启"
  else u8g2.print("窗帘:关闭");  // 否则显示"窗帘:关闭"
  u8g2.setCursor(0,60);  // 设置光标位置为第四行
  if(state.condition_state==1)u8g2.print("空调:开启");  // 如果空调打开则显示"空调:开启"
  else u8g2.print("空调:关闭");  // 否则显示"空调:关闭"
  }  // 结束else分支

  u8g2.sendBuffer();   // 将缓冲区内容发送到OLED显示屏进行显示
 
 
}
/**
 * @brief 按键扫描函数
 * @return 按键值 (0: 无按键, 1: 按键1, 2: 按键2)
 * @details 检测 GPIO13 和 GPIO15 的按键状态，实现按键去抖动
 */
int key_scnaf(){
  static int key_state=0;  // 静态变量，保存按键状态机状态
  int key_return=0;  // 返回的按键值
  switch (key_state)  // 使用状态机处理按键扫描
  {
    case 0:  // 初始状态，无按键
      if(digitalRead(13)==LOW&&digitalRead(15)==LOW)key_state=0;  // 如果两个按键脚都为低电平，保持初始状态
      else key_state=1;  // 如果任一按键脚为高电平，进入检测状态
      break;  // 结束case 0
    case 1:  // 检测状态
      if(digitalRead(13)==LOW&&digitalRead(15)==LOW)key_state=0;  // 如果两个按键脚都为低电平，回到初始状态
      else {  // 否则进入按键判断
        if(digitalRead(13)==HIGH)key_return=1;  // 如果GPIO13脚为高电平，则检测到按键1被按下
        if(digitalRead(15)==HIGH)key_return=2;  // 如果GPIO15脚为高电平，则检测到按键2被按下
        key_state=2;  // 进入按键释放等待状态
      }  // 结束else分支
    break;  // 结束case 1
    case 2:  // 按键释放等待状态
      if(digitalRead(13)==LOW&&digitalRead(15)==LOW)key_state=0;  // 等待两个按键脚都为低电平，表示按键释放完成，回到初始状态
  }  // 结束switch语句
  return key_return;  // 返回按键值（0表示无按键，1表示按键1，2表示按键2）
}  // 结束按键扫描函数
/**
 * @brief 按键处理函数
 * @details 处理按键事件：
 * - 按键1: 请求获取温湿度数据
 * - 按键2: 请求获取设备状态，并切换 OLED 显示到状态页面
 */
void key_pro(){
  time1=millis();  // 获取当前系统时间（毫秒）
  if(time1-key_time<10)return;  // 检查距离上次按键是否小于10ms，如果是则返回（去抖动处理）
  key_time=time1;  // 更新按键时间戳到当前时间
  key_value=key_scnaf();  // 调用按键扫描函数获取按键值
  if(key_value==1){  // 如果扫描到按键1被按下
    client.publish(topic_pub_command,"get_dht");  // 发布"get_dht"命令到MQTT获取温湿度数据
    key_value=0;  // 重置按键值为0
  }else if(key_value==2){  // 如果扫描到按键2被按下
    client.publish(topic_pub_command,"get_state");  // 发布"get_state"命令到MQTT获取设备状态
    oled_state=1;  // 切换OLED显示状态为1，用于显示设备状态页面
    oled_time=millis();  // 记录当前时间用于5秒后自动切换回温湿度显示
    key_value=0;  // 重置按键值为0
  }  // 结束else if分支
}  // 结束按键处理函数
/**
 * @brief Arduino setup 函数
 * @details 初始化系统：
 * - 初始化 OLED 显示屏
 * - 初始化串口通信
 * - 连接 WiFi
 * - 配置 MQTT 服务器和回调函数
 * - 设置按键引脚模式
 * - 初始化软件串口
 */
void setup() {
  u8g2.begin();  // 初始化OLED显示屏
  Serial.begin(115200);  // 初始化串口通信，波特率115200
  Serial.println("esp8266准备就绪");  // 输出准备就绪信息
  setup_wifi();  // 调用WiFi连接函数
  client.setServer(mqtt_server,1883);  // 设置MQTT服务器地址和端口
  client.setCallback(callback);  // 设置MQTT消息回调函数
  u8g2.enableUTF8Print();  // 启用UTF8打印支持
  pinMode(13,INPUT);  // 设置GPIO13为输入模式（按键1）
  pinMode(15,INPUT);  // 设置GPIO15为输入模式（按键2）
  mySerial.begin(9600);  // 初始化软件串口，波特率9600
}


/**
 * @brief Arduino 主循环函数
 * @details 持续运行的主要逻辑：
 * - 保持 MQTT 连接
 * - 处理 MQTT 消息
 * - 处理按键输入
 * - 处理来自 Mega2560 的串口数据并转换为 MQTT 命令
 * - 更新 OLED 显示
 */
void loop() {
  if(!client.connected()){  // 如果MQTT未连接
    reconnect();  // 重新连接MQTT
  }
  client.loop();  // 处理MQTT消息
  key_pro();  // 处理按键输入
  if(mySerial.available() >= 4){  // 检查软件串口是否有至少4个字节数据
    byte buffer[4];  // 创建4字节缓冲区存储接收的数据
    mySerial.readBytes(buffer, 4);  // 从软件串口读取4个字节的数据
    Serial.print("收到指令码: ");  // 输出接收到指令码的提示信息
    for(int i=0; i<4; i++){  // 遍历4个字节的数据
      if(buffer[i] < 0x10) Serial.print("0");  // 如果字节值小于0x10，补零
      Serial.print(buffer[i], HEX);  // 以十六进制格式打印字节值
      Serial.print(" ");  // 打印空格分隔符
    }
    Serial.println();  // 打印换行符
    if(buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00){  // 检查前三个字节是否都为0x00
      switch(buffer[3]){  // 根据第四个字节的值进行相应处理  // 根据第四个字节的值进行相应处理
        case 0x01:  // 当指令为0x01时执行LED开启
          client.publish(topic_pub_command, "led_on");  // 发布LED开启命令到MQTT
          Serial.println("执行: LED开");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x02:  // 当指令为0x02时执行LED关闭
          client.publish(topic_pub_command, "led_off");  // 发布LED关闭命令到MQTT
          Serial.println("执行: LED关");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x03:  // 当指令为0x03时执行风扇开启
          client.publish(topic_pub_command, "fan_on");  // 发布风扇开启命令到MQTT
          Serial.println("执行: 风扇开");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x04:  // 当指令为0x04时执行风扇关闭
          client.publish(topic_pub_command, "fan_off");  // 发布风扇关闭命令到MQTT
          Serial.println("执行: 风扇关");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x05:  // 当指令为0x05时执行窗帘开启（舵机开）
          client.publish(topic_pub_command, "servo_on");  // 发布舵机开启命令到MQTT
          Serial.println("执行: 窗帘开");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x06:  // 当指令为0x06时执行窗帘关闭（舵机关）
          client.publish(topic_pub_command, "servo_off");  // 发布舵机关闭命令到MQTT
          Serial.println("执行: 窗帘关");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x07:  // 当指令为0x07时执行空调开启
          client.publish(topic_pub_command, "condition_on");  // 发布空调开启命令到MQTT
          Serial.println("执行: 空调开");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x08:  // 当指令为0x08时执行空调关闭
          client.publish(topic_pub_command, "condition_off");  // 发布空调关闭命令到MQTT
          Serial.println("执行: 空调关");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x09:  // 当指令为0x09时执行全部开启
          client.publish(topic_pub_command, "all_on");  // 发布全部开启命令到MQTT
          Serial.println("执行: 全开");  // 输出执行信息
          break;  // 跳出switch语句
        case 0x0A:  // 当指令为0x0A时执行全部关闭
        case 0x10:  // 当指令为0x10时也执行全部关闭
          client.publish(topic_pub_command, "all_off");  // 发布全部关闭命令到MQTT
          Serial.println("执行: 全关");  // 输出执行信息
          break;  // 跳出switch语句
        default:  // 其他指令值
          Serial.println("收到未知指令尾缀");  // 输出未知指令提示
          break;  // 跳出switch语句
      }
      client.publish(topic_pub_command, "get_state");
      oled_state=1;  // 切换OLED显示状态为1，用于显示设备状态页面
      oled_time=millis();   // 发布状态查询命令获取最新状态
    }  // 结束串口数据接收条件判断
  }  // 结束串口数据可用的条件判断
  oled_pro();  // 处理OLED显示屏更新
}  // 结束主循环函数