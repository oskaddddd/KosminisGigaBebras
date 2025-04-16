#include  <Arduino.h>


const uint8_t pp = A6;

void setup() {
    Serial.begin(9600);  // Connect to PC
    pinMode(pp, INPUT); 
}


void loop() {
  Serial.println(analogRead(pp));
  delay(100);
}