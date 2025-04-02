#include <DHT11.h>

DHT11 dht11(6);

void setup(){
  Serial.begin(9600);

}

void loop(){
  int temp = dht11.readTemperature();
  int hum = dht11.readHumidity();

  Serial.println(temp);

  Serial.println(hum);

  
}