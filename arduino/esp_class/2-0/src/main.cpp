#include <Arduino.h>

#include<DHT.h>
float humidity=0;
float temperature=0;
DHT dht(13,DHT11);
void setup() {
  
  dht.begin();
  Serial.begin(115200);
}

void loop() {
  delay(1000);
  humidity=dht.readHumidity();
  temperature=dht.readTemperature();
  Serial.println("humidity:"+String(humidity));
  Serial.println("temperature:"+String(temperature));
}


