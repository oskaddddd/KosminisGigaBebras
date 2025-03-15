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

  //Wait till radio initialises
  while(!radioModule.init()){
    delay(5000);
  }

  //Setup radio
  radioModule.setAddress(230,true);
  Serial.print("Adress: ");
  radioModule.setChannel(123,123);
  Serial.println(radioModule.getAddress());
  radioModule.printBoardParameters();

  //Wait for handshake and respond once recieved
  while(true){
    if (Serial.available()){
      if (Serial.read() == 0x69){
        Serial.print(0x69);
        break;
      }
    }
    delay(1000);
  }

}

void loop() {
  if (KosminesGigaBebroBangos.available()){
    if (KosminesGigaBebroBangos.read() == 0x00) {  
      byte buffer[63];

      //read bytes into a buffer
      int bytesRead = KosminesGigaBebroBangos.readBytes(buffer, sizeof(buffer));

      //print a line of recieved bytes and an eol to mark the end of stream
      for (int i=0; i<bytesRead; i++){
        Serial.print(buffer[i]);
      }
      Serial.print("\n");
    }
  }
  delay(50);
}