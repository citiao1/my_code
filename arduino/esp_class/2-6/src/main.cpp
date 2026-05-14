#include <Arduino.h>
#include<DHT.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
LiquidCrystal_I2C lcd(0x27,16,2);
float humidity=0;
float temperature=0;
long time1=0;
long lcd_time=0;
long dht_time=0;
DHT dht(13,DHT11);
void dht_app(){
  time1=millis();
  if(time1-dht_time<200)return;
  dht_time=time1;
  humidity=dht.readHumidity();
  temperature=dht.readTemperature();
  Serial.println("humidity:"+String(humidity,2));
  Serial.println("temperature:"+String(temperature,2));
}
void lcd_app(){
  time1=millis();
  if(time1-lcd_time<10)return;
  lcd_time=time1;
  lcd.setCursor(0,0);
  lcd.print("HUMI:"+String(humidity,2)+"%");  
  lcd.setCursor(0,1);
  lcd.print("TEMP:"+String(temperature,2)+"C");

}
void setup() {
  lcd.init();
  Serial.begin(115200);
  lcd.backlight();
  dht.begin();
}

void loop() {
  dht_app();
  lcd_app();
}

