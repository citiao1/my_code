long time=0;
long blink_time=0;
int blink_mask=0;
int blink_state=0;
long key_time=0;
void blink(int delay_time){
  time=millis();
  if(time-blink_time<delay_time)return;
  blink_time=time;
  if(blink_mask){
    digitalWrite(2, HIGH);
  }else {
    digitalWrite(2, LOW);
  }
  blink_mask^=1;
}
void key(){
  time=millis();
  if(time-key_time<10)return;
  key_time=time;
  if(digitalRead(3)==HIGH){
    blink_state^=1;
    while(digitalRead(3)==HIGH);
  }
}
void setup() {
  pinMode(2,OUTPUT);// put your setup code here, to run once:
  pinMode(3,INPUT);
}

void loop() {
key();
if(blink_state){
  blink(500);
}else{
  digitalWrite(2,LOW);
}

}
