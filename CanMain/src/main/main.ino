//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>

#include  <HardwareSerial.h> 
#include  <SoftwareSerial.h>

//#include  <cstdint>
#include <UbxGpsNavPvt.h>


#include <TinyGPS++.h>
#include "DHT.h"

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
  double gps[3] {};                 // 12 bytes (3 * 4 bytes)
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

const byte rx_gps = 2;
const byte tx_gps = 3;

SoftwareSerial gps_serial(rx_gps, rx_gps);

#define DHT11_PIN 5
UbxGpsNavPvt<SoftwareSerial> gps(gps_serial);


GY_85 GY85;
DHT dht11(DHT11_PIN, DHT11);






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


const float lpAlpha = 0.7;
void readSensors() {

  int* acc1 = GY85.readFromAccelerometer();
  delay(50);
  int* acc2 = GY85.readFromAccelerometer();
  int avgAcc[3] {};

  avgAcc[0] = (GY85.accelerometer_x(acc1) + GY85.accelerometer_x(acc2))/2;
  avgAcc[1] = (GY85.accelerometer_y(acc1) + GY85.accelerometer_y(acc2))/2;
  avgAcc[2] = (GY85.accelerometer_z(acc1) + GY85.accelerometer_z(acc2))/2;
  if (!data.acceleration){
    data.acceleration[0] = avgAcc[0]*100;
    data.acceleration[1] = avgAcc[1]*100;
    data.acceleration[2] = avgAcc[2]*100;  
  }
  else{
    data.acceleration[0] = data.acceleration[0]*lpAlpha+(avgAcc[0]*(1.0-lpAlpha)*100);
    data.acceleration[1] = data.acceleration[1]*lpAlpha+(avgAcc[1]*100*(1.0-lpAlpha)*100);
    data.acceleration[2] = data.acceleration[2]*lpAlpha+(avgAcc[2]*(1.0-lpAlpha)*100);
  }
  
  
  int* compassReadings = GY85.readFromCompass();
  data.angVelocity[0] = GY85.compass_x(compassReadings)*100;
  data.angVelocity[1] = GY85.compass_y(compassReadings)*100;
  data.angVelocity[2] = GY85.compass_z(compassReadings)*100;
 

  float* gyroReadings = GY85.readGyro();
  data.angVelocity[0] = GY85.gyro_x(gyroReadings);
  data.angVelocity[1] = GY85.gyro_y(gyroReadings);
  data.angVelocity[2] = GY85.gyro_z(gyroReadings);
  data.temperature = GY85.temp(gyroReadings);

    // read humidity
  float humi  = dht11.readHumidity();
  // read temperature as Celsius
  float tempC = dht11.readTemperature();
  // read temperature as Fahrenheit
  float tempF = dht11.readTemperature(true);

  data.gps[0] = gps_serial.available()+1;
  Serial.print("1");
  
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
  if (gps.ready())
    {
        Serial.print(gps.lon / 10000000.0, 7);
        Serial.print(',');
        Serial.print(gps.lat / 10000000.0, 7);
        Serial.print(',');
        Serial.print(gps.height / 1000.0, 3);
        Serial.print(',');
        Serial.println(gps.gSpeed * 0.0036, 5);
    }
  return;
  readSensors();
  BuildPacket(0);
  //Serial.write(Packet, packetLength);
  //SendPacket();
  //delay(300);
  
  //BuildPacket(1);
  //SendPacket();
}