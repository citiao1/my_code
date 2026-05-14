#include <Arduino.h>
// 休止符
#define NOTE_REST 0

// 中音区 (无点)
#define NOTE_C4  262 // Do (1)
#define NOTE_D4  294 // Re (2)
#define NOTE_E4  330 // Mi (3)
#define NOTE_F4  349 // Fa (4)
#define NOTE_FS4 370 // 升Fa (#4)
#define NOTE_G4  392 // Sol (5)
#define NOTE_GS4 415 // 升Sol (#5)
#define NOTE_A4  440 // La (6)
#define NOTE_B4  494 // Si (7)

// 高音区 (上面带一个点)
#define NOTE_C5  523 // 高音 Do (i)
#define NOTE_D5  587 // 高音 Re
#define NOTE_E5  659 // 高音 Mi
#define NOTE_F5  698 // 高音 Fa
#define NOTE_G5  784 // 高音 Sol
#define NOTE_A5  880 // 高音 La
#define NOTE_B5  988 // 高音 Si

int melody[] = {
  // 第 1-5 小节
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_E5,
  NOTE_G5, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_E5, NOTE_A5,
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5,

  // 第 6-10 小节
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_E5,
  NOTE_G5, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_E5, NOTE_A5,
  NOTE_B4, NOTE_GS4, NOTE_FS4, NOTE_E4,
  // 注：谱面第9-10小节的4和5未标升号，这里严格按谱面无升号(F4, G4)转换
  NOTE_B4, NOTE_GS4, NOTE_FS4, NOTE_E4, NOTE_F4, NOTE_G4,
  NOTE_B4, NOTE_GS4, NOTE_FS4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_F4, NOTE_G4,

  // 第 11-15 小节
  NOTE_B4, NOTE_GS4, NOTE_B4, NOTE_C5, NOTE_A4, NOTE_C5,
  NOTE_D5, NOTE_B4, NOTE_A4, NOTE_G4,
  NOTE_D5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_B4,
  NOTE_D5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_B4,
  NOTE_D5, NOTE_B4, NOTE_G4, NOTE_REST,

  // 第 16-20 小节
  NOTE_F5, 
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_E5,
  NOTE_G5, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_E5, NOTE_A5,
  NOTE_A5, NOTE_F5, NOTE_E5, NOTE_D5,

  // 第 21-25 小节
  NOTE_A5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_E5, NOTE_F5,
  NOTE_A5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_E5, NOTE_F5,
  // 注：第23小节后半段谱面 7 4 7 未标高音点，严格按谱面提取
  NOTE_A5, NOTE_F5, NOTE_A5, NOTE_B4, NOTE_F4, NOTE_B4,
  NOTE_B4, NOTE_G4, NOTE_B4, NOTE_C5, NOTE_G4, NOTE_C5,
  NOTE_C5, NOTE_A4, NOTE_C5, NOTE_D5, NOTE_A4, NOTE_D5,

  // 第 26-29 小节
  NOTE_E5,
  NOTE_D5, NOTE_C5,
  NOTE_B4,
  NOTE_B4,

  // 第 30-33 小节 (回到了中音区)
  NOTE_G4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_G4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_E4,
  NOTE_G4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_C4
};
long time1=0;
long buzzer_time=0;
int count=0;
int note=0;
void buzzer_app(){
  time1=millis();
  if(time1-buzzer_time<500)return;
  buzzer_time=time1;
  tone(12,melody[count]);
  Serial.println(melody[count]);
  count++;
  if(count>=note)count=0;
}
void setup() {
  Serial.begin(115200);
  pinMode(12,OUTPUT);
  note=sizeof(melody)/sizeof(melody[0]);
}

void loop() {
  buzzer_app();
}

