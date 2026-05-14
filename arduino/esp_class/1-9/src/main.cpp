#include <Arduino.h>

// put function declarations here:
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels=Adafruit_NeoPixel(4,13,NEO_GRB+NEO_KHZ800);
int count=0;
void setup() {
  Serial.begin(115200);
  pinMode(13,OUTPUT);
  pixels.begin();
}

void loop() {
   pixels.setBrightness(255);
    for(int i=0;i<4;i++){
    switch (i)
    {
    case 0:
      pixels.setPixelColor(0,255,0,0);
      pixels.setPixelColor(1,0,255,0);
      pixels.setPixelColor(2,0,0,255);
      pixels.setPixelColor(3,255,255,255);
      break;
    case 1:
      pixels.setPixelColor(1,255,0,0);
      pixels.setPixelColor(2,0,255,0);
      pixels.setPixelColor(3,0,0,255);
      pixels.setPixelColor(0,255,255,255);
    break;
    case 2:
      pixels.setPixelColor(2,255,0,0);
      pixels.setPixelColor(3,0,255,0);
      pixels.setPixelColor(0,0,0,255);
      pixels.setPixelColor(1,255,255,255);
      break;
    case 3:
      pixels.setPixelColor(3,255,0,0);
      pixels.setPixelColor(2,0,255,0);
      pixels.setPixelColor(1,0,0,255);
      pixels.setPixelColor(0,255,255,255);
      break;
    default:
      break;
    }
    pixels.show();
    count++;
    Serial.println(count);
    delay(2000);
  }
  pixels.show();
}
