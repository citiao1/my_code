// 定义摇杆引脚（对应你的接线）'
#include <Arduino.h>
const int pinX = A9;
const int pinY = A8;
const int pinSW = 2;

void setup() {
  // 1. 初始化串口通信，波特率 9600
  Serial.begin(9600);
  
  // 2. 配置引脚
  // 输入模式，按键必须开启上拉电阻(INPUT_PULLUP)，否则读数会乱跳
  pinMode(pinX, INPUT);
  pinMode(pinY, INPUT);
  pinMode(pinSW, INPUT_PULLUP);
  
  Serial.println("摇杆测试开始...请推动摇杆");
}

void loop() {
  // 3. 读取原始数据 (0-1023)
  int valX = analogRead(pinX);
  int valY = analogRead(pinY);
  int valSW = digitalRead(pinSW); // 1是松开，0是按下

  // 4. 打印到串口监视器
  Serial.print("X轴: ");
  Serial.print(valX);
  
  // 为了美观对齐，加个制表符
  Serial.print("\t Y轴: ");
  Serial.print(valY);
  
  Serial.print("\t 按钮: ");
  // 如果读到0说明按下了
  if(valSW == 0) {
    Serial.print("已按下");
  } else {
    Serial.print("松开");
  }
  
  Serial.println(); // 换行
  
  delay(200); // 慢一点打印，方便看清
}