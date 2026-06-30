
#include <GD5800_Serial.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

GD5800_Serial mp345(4, 5);
MD_Parola P = MD_Parola(11, 13, 9, 1);
MD_MAX72XX mx = MD_MAX72XX(11, 13, 9, 1);

int love;
int item;

const uint8_t normal[8]={0xff,0x01,0x0a,0x22,0x22,0x0a,0x01,0xff,};
const uint8_t normal1[8]={0xff,0x01,0x0a,0x42,0x42,0x0a,0x01,0xff,};
const uint8_t sad[8]={0xff,0x01,0x4a,0x22,0x22,0x4a,0x01,0xff,};
const uint8_t well[8]={0xff,0x01,0x2a,0x42,0x42,0x2a,0x01,0xff,};
const uint8_t smile[8]={0xff,0x01,0x2a,0x62,0x62,0x2a,0x01,0xff,};
const uint8_t love1[8]={0xff,0x31,0x4a,0x92,0x92,0x4a,0x31,0xff,};
const uint8_t love2[8]={0xff,0x21,0x52,0xa2,0xa2,0x52,0x21,0xff,};
const uint8_t eat1[8]={0xff,0x01,0x0a,0x62,0x62,0x0a,0x01,0xff,};
const uint8_t eat2[8]={0xff,0x01,0x4a,0xa2,0xa2,0x4a,0x01,0xff,};

void setup(){
  mp345.begin(9600);
  mp345.setVolume(255);
  Serial.begin(9600);
  mx.begin();
  P.begin();
  love = 0;
  item = 0;
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  
}

void loop(){
  P.displayAnimate();
  if (digitalRead(6) == false) {
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setBuffer(7, 8, normal);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    delay(500);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setBuffer(7, 8, normal1);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    delay(500);

  }
  if (love >= 3 && digitalRead(6) == false) {
    mp345.playFileByIndexNumber(5);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setBuffer(7, 8, love1);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    delay(500);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setBuffer(7, 8, love2);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    delay(500);
    love = 0;

  }
  if (digitalRead(7) == true) {
    item = random(1, 4);
    if (item == 1) {
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      mx.setBuffer(7, 8, smile);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mp345.playFileByIndexNumber(3);
      delay(1000);
      love = love + 2;

    } else if (item == 2) {
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      mx.setBuffer(7, 8, sad);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mp345.playFileByIndexNumber(2);
      delay(1000);
      love = love - 1;
    } else {
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      mx.setBuffer(7, 8, well);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mp345.playFileByIndexNumber(1);
      delay(1000);
      love = love + 1;

    }
    Serial.println(item);

  }
  if (digitalRead(6) == true) {
    mp345.playFileByIndexNumber(4);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setBuffer(7, 8, eat1);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    delay(500);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setBuffer(7, 8, eat2);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    delay(500);
    love++;

  }


}