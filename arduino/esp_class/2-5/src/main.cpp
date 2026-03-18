#include <Arduino.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
char* str="A2401 fusixiong 2024030448 wangzheyuan 2024030486";
int lenth=0;
int left_count=0;
int right_count=0;
long time1=0;
long lcd_time=0;
int lcd_state=0;
int return_count=0;
void lcd_leftdisplay(char* str){
  time1=millis();
  if(time1-lcd_time<200)return;
  lcd_time=time1;
  lcd.scrollDisplayLeft();
  left_count++;
  if(left_count>=strlen(str)){
    left_count=0;
    lcd_state=1;
  }
}

void lcd_rightdisplay(char* str){
  time1=millis();
  if(time1-lcd_time<200)return;
  lcd_time=time1;
  lcd.scrollDisplayRight();
  right_count++;
  if(right_count>=16+strlen(str)){
    right_count=0;
    lcd_state=2;
  }
}
void lcd_returndisplay(char* str){
  time1=millis();
  if(time1-lcd_time<200)return;
  lcd_time=time1;
  lcd.scrollDisplayLeft();
  left_count++;
  if(return_count>=16){
    return_count=0;
    lcd_state=0;
  }
}
void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print(str);

}

void loop() {
  switch (lcd_state)
  {
  case 0:
    lcd_leftdisplay(str);
    break;
  case 1:
    lcd_rightdisplay(str);
    break;
  case 2:
    lcd_returndisplay(str);
    break;
  default:
    break;
  }
}
