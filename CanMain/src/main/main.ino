//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>

#include  <HardwareSerial.h> 
#include  <SoftwareSerial.h>

//#include  <cstdint>

#include <TinyGPSPlus.h>


//Sensors
#include "GY_85.h"


#pragma pack(push, 1) 
struct PacketHeader {
  uint8_t start[2] = {0x98, 0x98}; 
  uint8_t senderId = 0xff;         
  uint8_t length {};
  uint8_t packetType {};                           
  uint32_t timestamp {};
  
  PacketHeader(){
    timestamp = millis();
  }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DataPayload {
  int16_t angVelocity[3] {};       // 2 bytes
  int16_t acceleration[3] {};      // 6 bytes (3 * 2 bytes)
  int16_t magneticField[3] {};     // 6 bytes (3 * 2 bytes)
  float gps[3] {};                 // 12 bytes (3 * 4 bytes)
  int16_t temperature {};          // 2 bytes
  //uint16_t pressure;            // 2 bytes
  //uint8_t humidity;             // 1 byte
  uint16_t vocConcentration {};    // 2 bytes
  uint16_t co2Concentration {};    // 2 bytes
};
#pragma pack(pop)

#pragma pack(push, 1) 
struct DebugPayload {
  uint16_t packetCount {};
  uint16_t batteryVoltage {};
  uint16_t memUsage {};
  uint8_t sensors {};

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


uint8_t packetLength = 0;
uint8_t Packet[64] = {};

#pragma pack(push, 1) 
struct PacketFooter {
  uint8_t checksum = 0x00;
  char endChar[2] = "\r\n";


  void CalcChecksum() {
    for (uint8_t i = 0; i < packetLength-sizeof(PacketFooter); i++) {
      checksum ^= Packet[i];
    }
  }
};
#pragma pack(pop)
DataPayload data;
DebugPayload debug;




TinyGPSPlus gps;
GY_85 GY85;



const byte rx_gps = 2;
const byte tx_gps = 3;
//const byte rx_lora = 5;
//const byte tx_lora = 4;


SoftwareSerial gps_serial(rx_gps, tx_gps);


const byte address =253;

char error[64] = "" ;
uint8_t errorLength = 0;



File file;

void setup() {
  
  
  //Setup Serial and LoRa
  Serial.begin(57600); //Rx Tx

  gps_serial.begin(115200);

  //Set up wire
  Wire.begin();

  delay(1000);

  GY85.init();

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

void RaiseError(char* message){
  //Get the error length and make sure it doesnt exeed limit
  errorLength = strlen(message);
  errorLength = (errorLength < 64) ? errorLength : 64;

  //Copy the message to the errorMessage to the error object
  memcpy(error, message, errorLength);
}

void ser_wait(int command_len){
  while (Serial.available() <= command_len){
    delay(100);
    Serial.print("waiting for commands {");Serial.print(command_len); Serial.print("} response");
  }
}
//Enter LoRa bootlader to config stuff
bool enterBootlader(bool i = 0){

  Serial.print("+++");
  ser_wait(3);

  if (Serial.readString() == "OK\r\n"){
    return 1;
  }
  //If Entering bootloader failed, restart Lora and try again once
  else if(i == 0){
    Serial.print("ATZ");
    delay(500);
    while(!Serial){delay(100);}
    return enterBootlader(1);
  }
  else{
    return 0;
  }

}
bool setAddressLocal(uint16_t address){
  while(!Serial){delay(100);}
  
  if (!enterBootlader()){
    RaiseError("setAddress: Enter bootloader failed");
  }
  Serial.print("ATS3=");
  Serial.println(address);
  int addressLen = 0;

  while(address/pow(10, addressLen)){
    addressLen++;
  }
  ser_wait(5+addressLen+2); 
  String resp = Serial.readString();

  if (resp.endsWith("OK\r\n")){
    Serial.println('AT&W');
  }
  else{return 0;}

  ser_wait(6);
  resp = Serial.readString();
  if (resp.endsWith("OK\r\n")){
    Serial.println('ATZ');
  }
  else{return 0;}


  delay(500);
  while(!Serial){delay(100);}

  return 1;
}



void readSensors() {

  int* accelerometerReadings = GY85.readFromAccelerometer();
  data.acceleration[0] = GY85.accelerometer_x(accelerometerReadings)*100;
  data.acceleration[1] = GY85.accelerometer_y(accelerometerReadings)*100;
  data.acceleration[2] = GY85.accelerometer_z(accelerometerReadings)*100;
  
  
  int* compassReadings = GY85.readFromCompass();
  data.angVelocity[0] = GY85.compass_x(compassReadings)*100;
  data.angVelocity[1] = GY85.compass_y(compassReadings)*100;
  data.angVelocity[2] = GY85.compass_z(compassReadings)*100;
 

  float* gyroReadings = GY85.readGyro();
  data.angVelocity[0] = GY85.gyro_x(gyroReadings);
  data.angVelocity[1] = GY85.gyro_y(gyroReadings);
  data.angVelocity[2] = GY85.gyro_z(gyroReadings);
  data.temperature = GY85.temp(gyroReadings);


  if (gps_serial.available() > 0) {
    if (gps.encode(gps_serial.read())) {
      if (gps.location.isValid()) {
        
        gps.location.lat();
        gps.location.lng();
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

void WriteToFile() {
  //file.write(Packet);
  
}






void BuildPacket(uint8_t type){
  PacketHeader header;
  PacketFooter footer;

  uint8_t fSize = sizeof(footer);
  uint8_t hSize = sizeof(header);

  //Data Packet
  if (type == 0){
    header.packetType = 0x00;

    uint8_t dSize = sizeof(data);

    packetLength = hSize + dSize+fSize;

    header.length = packetLength;

    //Copy data to the packet
    memcpy(Packet, &header, hSize);
    memcpy(Packet + hSize, &data, dSize);
  }
  //Debug packet
  else if (type == 1){
    header.packetType = 0x01;

    uint8_t dSize = sizeof(debug);

    //Adjust the error to it's MaxLength
    uint8_t maxErrorLength = 63 - (hSize + dSize);
    errorLength = (errorLength <= maxErrorLength) ? errorLength : maxErrorLength;

    packetLength = hSize + dSize + errorLength +fSize; 

    header.length = packetLength;

    //Copy data to the packet
    memcpy(Packet, &header, hSize);
    memcpy(Packet + hSize, &debug, dSize);
    memcpy(Packet + hSize + dSize, &error, errorLength);

  }

  //calculate the checksum
  footer.CalcChecksum();
  
  memcpy(Packet+(packetLength-fSize), &footer, fSize);

    
}

void SendPacket(){

  Serial.write(Packet, packetLength);
  memset(Packet, 0, sizeof(Packet));;

}

void loop() {
  
  //writeToFile();
  //packets();
  readSensors();
  BuildPacket(0);
  //Serial.write(Packet, packetLength);
  SendPacket();
  delay(2000);
  
  //BuildPacket(1);
  //SendPacket();
}