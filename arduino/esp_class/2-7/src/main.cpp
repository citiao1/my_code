#include <Arduino.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
byte char_ren[8]{0x04,0x08,0x14,0x04,0x04,0x04,0x04,0x04};
byte char_fu[8]{0x04,0x1F,0x04,0x14,0x04,0x14,0x0C,0x04};
byte char_si[8]{0x1F,0x15,0x1F,0x15,0x1F,0x00,0x15,0x0E};
byte char_xiong1[8]{0x04,0x1F,0x04,0x09,0x0A,0x15,0x17,0x00};
byte char_xiong2[8]{0x04,0x00,0x1F,0x04,0x1F,0x04,0x04,0x1F};
byte char_wang[8]{0x00,0x1F,0x04,0x04,0x1F,0x04,0x04,0x1F};
byte char_zhe[8]{0x0A,0x1C,0x0F,0x0C,0x18,0x0F,0x1D,0x0F};
byte char_yuan[8]{0x00,0x07,0x12,0x07,0x12,0x15,0x19,0x1F};
int left_count = 16;
int right_count = 0;
long time1 = 0;
long lcd_time = 0;
int lcd_state = 0;
int return_count = 0;
int lenth=12;
int left_flag=1;
void lcd_leftdisplay(int lenth){
  time1=millis();
  if(left_flag==1){
  if(time1-lcd_time<1000)return;}
  else{
    if(time1-lcd_time<10)return;
    if(left_count>=40){
      left_flag=1;
      left_count=0;  
    }

  }
  lcd_time=time1;
  lcd.scrollDisplayLeft();
  left_count++;
  Serial.println(left_count);
  if(left_count>=16+lenth){
    left_flag=0;
  }
}

/*void lcd_rightdisplay(int lenth){
  time1=millis();
  if(time1-lcd_time<500)return;
  lcd_time=time1;
  lcd.scrollDisplayRight();
  right_count++;
  if(right_count>=16 + lenth){
    right_count=0;
    lcd_state=2;
  }
}
void lcd_returndisplay(int lenth){
  time1=millis();
  if(time1-lcd_time<500)return;
  lcd_time=time1;
  lcd.scrollDisplayLeft();
  return_count++;
  if(return_count>=16){
    return_count=0;
    delay(1000);
    lcd_state=0;
    
  }
}*/
void setup() {
  lcd.init();
  Serial.begin(115200);
  lcd.backlight();
  lcd.clear();
  lcd.createChar(0,char_ren);
  lcd.createChar(1,char_fu);
  lcd.createChar(2,char_si);
  lcd.createChar(3,char_xiong1);
  lcd.createChar(4,char_xiong2);
  lcd.createChar(5,char_wang);
  lcd.createChar(6,char_zhe);
  lcd.createChar(7,char_yuan);
  lcd.setCursor(0,0);
  lcd.print("A2401");
  lcd.write(0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(0);
  lcd.write(4);
  lcd.setCursor(0,1);
  lcd.print("A2401");
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);  
}

void loop() {
  switch (lcd_state)
  {
  case 0:
    lcd_leftdisplay(lenth);
    break;
  case 1:
    //lcd_rightdisplay(lenth);
    break;
  case 2:
    //lcd_returndisplay(lenth);
    break;
  default:
    break;
  }// put your main code here, to run repeatedly:
}

// put function definitions here:
