//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>

#include  <HardwareSerial.h> 
#include  <SoftwareSerial>

#include  <cstdint>

#include <TinyGPS++.h>




//Sensors
#include "GY_85.h"


#pragma pack(push, 1) 
struct PacketHeader {
  uint8_t startByte = 0x00;          // 1 byte
  uint8_t length;
  uint8_t packetId;                  // 1 byte
  uint16_t senderId = 0xe6;          // 1 byte
  uint32_t timestamp;
  
  PacketHeader(){
    timestamp = millis();
  }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DataPayload {
  int16_t angVelocity[3];       // 2 bytes
  int16_t acceleration[3];      // 6 bytes (3 * 2 bytes)
  int16_t magneticField[3];     // 6 bytes (3 * 2 bytes)
  float gps[3];                 // 12 bytes (3 * 4 bytes)
  int16_t temperature;          // 2 bytes
  uint16_t pressure;            // 2 bytes
  uint8_t humidity;             // 1 byte
  uint16_t vocConcentration;    // 2 bytes
  uint16_t co2Concentration;    // 2 bytes
};
#pragma pack(pop)

#pragma pack(push, 1) 
struct DebugPayload {
  uint16_t packetCount;
  uint16_t batteryVoltage;
  uint16_t memUsage;
  uint8_t sensors;

  void setSensorStatus(int index, bool status) {
    //On
    if (status) {
      sensors |= (1 << index);  //Shift the 1 to the index, and use OR 
    } 
    //Off
    else {
      sensors &= ~(1 << index);  //Shift the 1 to the index, invert with not, and use AND
    }
  }
};
#pragma pack(pop)

DataPayload data = {};
DebugPayload debug = {};

HardwareSerial LoRa_serial(1);

LoRa

GY_85 GY85;


const byte rxPin = 16;
const byte txPin = 17;

const int DHT_pin = 5;

const byte channel = 230;
const byte address[2] = {123, 123};

char error[64] = "" ;
uint8_t errorLength = 0;

uint8_t packetLength = 0;
uint8_t Packet[64] = {};

File file;

void setup() {
  
  
  //Setup Serial and LoRa
  Serial.begin(9600);
  LoRa_serial.begin(9600, SERIAL_8N1, rxPin, txPin); //Rx Tx

  //Set up wire
  if(!Wire.begin()){
    Serial.print("Failed to inicialize wire");
  }

  delay(1000);

  GY85.init();

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }


  if (!SD.begin()) {
    Serial.println("SD card failed");
    return;
  }


  //Test if SD card is working
  file = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  if (!file) {
    Serial.println("SD card failed");
  }

}

bool setAddress(uint16_t address){
  return 1
}
bool setChannel(uint16_t channel){
  return 1
}

void readSensors() {

  float check;

  int* accelerometerReadings = GY85.readFromAccelerometer();
  int ax = GY85.accelerometer_x(accelerometerReadings)*100;
  int ay = GY85.accelerometer_y(accelerometerReadings)*100;
  int az = GY85.accelerometer_z(accelerometerReadings)*100;
  
  int* compassReadings = GY85.readFromCompass();
  int cx = GY85.compass_x(compassReadings)*100;
  int cy = GY85.compass_y(compassReadings)*100;
  int cz = GY85.compass_z(compassReadings)*100;

  float* gyroReadings = GY85.readGyro();
  float gx = GY85.gyro_x(gyroReadings);
  float gy = GY85.gyro_y(gyroReadings);
  float gz = GY85.gyro_z(gyroReadings);
  float gt = GY85.temp(gyroReadings);
  //Get data.humidity data
  DHT_sensor.read(DHT_pin);
  data.humidity = DHT_sensor.humidity;

  //Read data.temperature (100*C) and check if its valid
  check = BMP_sensor.readTemperature();
  if (check == NAN){
    data.temperature = (DHT_sensor.temperature*100);
  }
  else{
    data.temperature = (check*100);
  }

  //Read preassure (Pa) and check if its valid
  check = BMP_sensor.readPressure();
  if (check == NAN){
    data.pressure = 0;
  }
  else{
    data.pressure = check;
  }

  if (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        
        gps.location.lat()
        gps.location.lng()
        if (gps.altitude.isValid())
          Serial.println(gps.altitude.meters());
        else
          Serial.println(F("INVALID"));
      } else {
        Serial.println(F("- location: INVALID"));
      }

      if (gps.speed.isValid()) {
        Serial.print(gps.speed.kmph());
        Serial.println(F(" km/h"));
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.println();
    }
  }


}

//Print data to file
template <typename... Args>
void WriteToFile(Args... args) {
  (file.print(args), ..., file.print(" "));  // Print each argument followed by a space
  file.println();
}

void RaiseError(char* message){
  //Get the error length and make sure it doesnt exeed limit
  errorLength = strlen(message);
  errorLength = (errorLength < 64) ? errorLength : 64;

  //Copy the message to the errorMessage to the error object
  memcpy(error, message, errorLength);
}



void BuildPacket(uint8_t type){
  PacketHeader header;

  uint8_t hSize = sizeof(header);

  //Data Packet
  if (type == 0){
    header.packetId = 0x00;

    uint8_t dSize = sizeof(data);

    packetLength = hSize + dSize + 1; // +1 for the Checksum

    header.length = packetLength -2; //-2 for the start and length byte

    //Copy data to the packet
    memcpy(Packet, &header, hSize);
    memcpy(Packet + hSize, &data, dSize);
  }
  //Debug packet
  else if (type == 1){
    header.packetId = 0x01;

    uint8_t dSize = sizeof(debug);

    //Adjust the error to it's MaxLength
    uint8_t maxErrorLength = 63 - (hSize + dSize);
    errorLength = (errorLength <= maxErrorLength) ? errorLength : maxErrorLength;

    packetLength = hSize + dSize + errorLength + 1; // +1 for the Checksum

    header.length = packetLength -2; //-2 for the start and length byte

    //Copy data to the packet
    memcpy(Packet, &header, hSize);
    memcpy(Packet + hSize, &debug, dSize);
    memcpy(Packet + hSize + dSize, &error, errorLength);

  }

  //calculate the checksum
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < packetLength-1; i++) {
    checksum ^= Packet[i];
  }

  //Copy the checksum to the packet
  Packet[packetLength-1] = checksum;
  
}

void SendPacket(){
  if(LoRa_serial.available()){
    LoRa_serial.write(Packet, packetLength);
  }
}

void loop() {

  //writeToFile();
  //packets();
  readSensors();
  BuildPacket(0);
  SendPacket();
  delay(100);
  BuildPacket(1);
  SendPacket();
}