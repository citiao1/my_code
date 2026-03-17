#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);



void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,2);
  lcd.print("2024030448");
  lcd.setCursor(1,0);
  lcd.print("fusixiong A2401");
  
}

void loop() {
  
}

