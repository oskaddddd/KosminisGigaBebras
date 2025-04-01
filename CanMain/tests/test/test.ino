#include  <SoftwareSerial.h>


const byte rx_gps = 2;
const byte tx_gps = 3;



SoftwareSerial gps_serial(rx_gps, rx_gps);

void setup() {
    Serial.begin(115200);  // Connect to PC
    gps_serial.begin(115200);   // GPS baud rate (change if needed)
}

void loop() {
    while (gps_serial.available()) {
        Serial.write(gps_serial.read());  // Forward GPS data to PC
    }
    while (Serial.available()) {
        gps_serial.write(Serial.read());  // Send PC commands to GPS
    }
}