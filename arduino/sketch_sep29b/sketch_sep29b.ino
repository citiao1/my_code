

void setup() {
  pinMode(2,OUTPUT);
  
  ledcAttach(2,15);
  // put your setup code here, to run once:

}

void loop() {
  ledcWrite(15,26);
  delay(1000);
  ledcWrite(15,77);
  delay(1000);
  // put your main code here, to run repeatedly:

}
