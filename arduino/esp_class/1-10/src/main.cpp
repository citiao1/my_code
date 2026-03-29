#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels=Adafruit_NeoPixel(4,13,NEO_GRB+NEO_KHZ800);
int serial_flag=0;
long time1=0;
long key_time=0;
long led_time=0;
int led_state=0;
int led_flag=0;
uint32_t colors[4]{
  pixels.Color(255,0,0),
  pixels.Color(0,255,0),
  pixels.Color(0,0,255),
  pixels.Color(255,255,255)
};

int led_color_index[4]={0,1,2,3};

void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  
  if(digitalRead(4)==HIGH){
    led_state=1;
    serial_flag=1;
    if(serial_flag==1){
      Serial.println("启动");
      serial_flag=0;
    }
    while(digitalRead(4)==HIGH);
  }
  if(digitalRead(5)==HIGH){
    led_state=0;
    serial_flag=1;
    if(serial_flag==1){
      Serial.println("关闭");
      for(int i=0;i<4;i++){
        pixels.setPixelColor(i,0,0,0);
        pixels.show();
      }
      serial_flag=0;
    }
    
    while(digitalRead(5)==HIGH);
  }
}
void led_app(){
    time1=millis();
    if(time1-led_time<2000)return;
    led_time=time1;
    pixels.clear();
    pixels.setBrightness(255);
    pixels.setPixelColor(led_flag,colors[led_color_index[led_flag]]);
    pixels.show();
    led_color_index[led_flag]++;
    if(led_color_index[led_flag]>=4)led_color_index[led_flag]=0;
    led_flag++;
    if(led_flag>3)led_flag=0;
    

    
  }
  


void setup() {
  pinMode(13,OUTPUT);
  Serial.begin(115200);
  pixels.begin();
  
}

void loop() {
  key_pro();
  if(led_state==1)led_app();
  
}

