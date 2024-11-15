#include <Arduino.h>
#include "E220.h"
#include <SoftwareSerial.h>
#include <Stream.h>
#define m0 6
#define m1 7
#define aux 5
const byte rxPin = 3;
const byte txPin = 2;
SoftwareSerial reciever (rxPin, txPin);
Stream &KosminesGigaBebroBangos = (Stream &)reciever;

void setup() {
  Serial.begin(9600);
  reciever.begin(9600); 
  E220 radioModule(&KosminesGigaBebroBangos, m0, m1, aux);
  while(!radioModule.init()){
      delay(5000);
  }
  radioModule.setAddress(230,true);
  Serial.print("Adress: ");
  Serial.println(radioModule.getAddress());
  radioModule.printBoardParameters();
}

void loop() {
  if (KosminesGigaBebroBangos.available()){
    if (KosminesGigaBebroBangos.read() == 0x00) {  
      byte buffer[63];
      int bytesRead = KosminesGigaBebroBangos.readBytes(buffer, sizeof(buffer));
      for (int i=0; i<bytesRead; i++){
        Serial.print(buffer[i]);
      }
    }
  }
}