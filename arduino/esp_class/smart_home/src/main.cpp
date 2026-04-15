#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

int speed=100;
float temperature=25.0;
float humidity=50.0;
int angle=90;
// 初始化 U8G2 (I2C 通信，ESP8266 默认 SDA 为 D2/GPIO4，SCL 为 D1/GPIO5)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  u8g2.begin();
  
  // 【关键步骤 1】开启 UTF-8 打印支持
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