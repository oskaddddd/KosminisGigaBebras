#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

SoftwareSerial gpsSerial(2, 3);  // RX, TX
TinyGPSPlus gps;

void setup() {
  Serial.begin(57600);
  gpsSerial.begin(115200); // try 9600 first
}

void loop() {
  while (gpsSerial.available()) {
    
    if(gps.encode(gpsSerial.read())){
      Serial.println(gps.location.isValid());
      Serial.println(gps.charsProcessed());
      Serial.println(gps.sentencesWithFix());
    }

  }
}
