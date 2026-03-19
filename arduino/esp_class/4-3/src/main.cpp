#include <Arduino.h>
#define NOTE_C4  262 // Do
#define NOTE_D4  294 // Re
#define NOTE_E4  330 // Mi
#define NOTE_F4  349 // Fa
#define NOTE_G4  392 // Sol
#define NOTE_A4  440 // La
#define NOTE_B4  494 // Si
long time1=0;
long buzzer_time=0;
int count=0;
int note=0;
int buzzer_state=0;
long key_time=0;
int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4,
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};
void buzzer_app(){
  time1=millis();
  if(time1-buzzer_time<500)return;
  buzzer_time=time1;
  tone(12,melody[count]);
  Serial.println(melody[count]);
  count++;
  if(count>=note)count=0;
}
void key_pro(){
  time1=millis();
  if(time1-key_time<10)return;
  key_time=time1;
  if(digitalRead(4)==HIGH){
    buzzer_state=1;
    while (digitalRead(4)==HIGH); 
  }
  if(digitalRead(5)==HIGH){
    buzzer_state=0;
    while(digitalRead(5)==HIGH);    
  }
}
void setup() {
  Serial.begin(115200);
  pinMode(4,INPUT);
  pinMode(5,INPUT);
  pinMode(12,OUTPUT);
}

void loop() {
  key_pro();
  if(buzzer_state==1)buzzer_app();
}

