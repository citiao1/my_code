#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels=Adafruit_NeoPixel(4,13,NEO_GRB+NEO_KHZ800);

void set4led(int r,int g,int b,int value){
  pixels.setBrightness(value);
  for(int i=0;i<4;i++){
    pixels.setPixelColor(i,r,g,b);
  }
  pixels.show();
}
void setup() {
  Serial.begin(115200);
  pinMode(13,OUTPUT);
  pixels.begin();
  // put your setup code here, to run once:
  
}

void loop() {
  Serial.println("red");
  set4led(255,0,0,255);
  delay(2000);
  
  Serial.println("green");
  set4led(0,255,0,255);
  delay(2000);
  
  Serial.println("blue");
  set4led(0,0,255,255);
  delay(2000);
  
}


