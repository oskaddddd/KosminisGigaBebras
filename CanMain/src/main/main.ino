//Libraries
//#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>

#include  <SoftwareSerial.h>

//#include  <cstdint>
#include <UbxGpsNavPvt.h>

#include "DHT_Async.h"

#include <TinyGPS++.h>

#define DHT_SENSOR_TYPE DHT_TYPE_11

static const int DHT_SENSOR_PIN = 6;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
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
  int32_t gps[2] {}; 
  uint16_t height {}; 
  int16_t velocity[3] {} ;           // 8 bytes (2 * 4 bytes)
  int16_t temperature {};           // 2 bytes
  uint8_t humidity {};             // 1 byte
  //uint16_t vocConcentration {};    // 2 bytes
  //uint16_t co2Concentration {};    // 2 bytes
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
uint8_t Packet[255] = {};

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


UbxGpsNavPvt<SoftwareSerial> gps(gps_serial);


GY_85 GY85;




const byte address =253;

char error[255] = "" ;
uint8_t errorLength = 0;



//File file;

void setup() {
  
  
  //Setup Serial and LoRa
  Serial.begin(57600); //Rx Tx

  gps.begin(115200);

  //Set up wire
  Wire.begin();

  delay(1000);

  GY85.init();


  delay(100);
  //if (!SD.begin()) {
  //  Serial.println("SD card failed");
  //  return;
  //}

  //Test if SD card is working
  //file = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  //if (!file) {
  //  Serial.println("SD card file");
  //}

}

void RaiseError(char* message){
  //Get the error length and make sure it doesnt exeed limit
  errorLength = strlen(message);
  errorLength = (errorLength < 255) ? errorLength : 255;

  //Copy the message to the errorMessage to the error object
  memcpy(error, message, errorLength);
}




const float lpAlpha = 0.7;

int* acc2 {};
int* acc1 {};
int* compassReadings {};
float* gyroReadings {};
int avgAcc[3] {};

void readSensors() {

  acc1 = GY85.readFromAccelerometer();
  delay(50);
  acc2 = GY85.readFromAccelerometer();

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
  
  
  compassReadings = GY85.readFromCompass();
  data.angVelocity[0] = GY85.compass_x(compassReadings)*100;
  data.angVelocity[1] = GY85.compass_y(compassReadings)*100;
  data.angVelocity[2] = GY85.compass_z(compassReadings)*100;
 

  gyroReadings = GY85.readGyro();
  data.angVelocity[0] = GY85.gyro_x(gyroReadings);
  data.angVelocity[1] = GY85.gyro_y(gyroReadings);
  data.angVelocity[2] = GY85.gyro_z(gyroReadings);
  
}




//Print data to file

//void WriteToFile() {
//  
//  file.write(Packet, packetLength);
//  
//}






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
  if (Serial.availableForWrite() >= packetLength){
  Serial.write(Packet, packetLength);
  memset(Packet, 0, sizeof(Packet));
  }


}



unsigned long time {};

float humidity {};
float temperature {};
int del = 300;

void loop() {

  if (dht_sensor.measure( &temperature, &humidity )) {
        data.temperature = temperature*100;
        data.humidity = humidity;
        //Serial.print("T = ");
        //Serial.print(temperature, 1);
        //Serial.print(" deg. C, H = ");
        //Serial.print(humidity, 1);
        //Serial.println("%");
    }
  if (time + del > millis()){
    //Special treatment for the gps cause hes a very special boy
    
    if (gps.ready())
    {   Serial.print("Hello?");
        Serial.println(gps.lon);
        data.gps[0] = gps.lon;
        data.gps[0] = gps.lat;
        data.height = gps.height/1000;
        
        data.velocity[0] = gps.velN/10;
        data.velocity[1] = gps.velE/10;
        data.velocity[2] = gps.velN/10;
    }
    return;
    
  }


  readSensors();
  BuildPacket(0);
  SendPacket();

  delay(30);
  time = millis();

}