//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>



//#include  <cstdint>
//#include <UbxGpsNavPvt.h>

#include "DHT_Async.h"

#include <TinyGPSPlus.h>

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
  uint32_t gps[2] {}; 
  uint16_t height {}; 
  int16_t velocity {};           // 8 bytes (2 * 4 bytes)
  int16_t temperature {};           // 2 bytes
  uint8_t humidity {};             // 1 byte
  //uint16_t vocConcentration {};    // 2 bytes
  //uint16_t co2Concentration {};    // 2 bytes
};
#pragma pack(pop)
#pragma pack(push, 1) 
struct DebugPayload {
  uint16_t packetCount = 1;
  float batteryVoltage {};
  uint16_t memUsage {};
  uint8_t sensors {};

  int getIndex(const char* device){
    if (device && *device){
      switch (device[strlen(device)-1]){
        //gY
        case 'y':
          return 0;
          break;
        //dhT
        case 't':
          return 1;
          break;
        //gpS
        case 's':
          return 2;
          break;
        //sD
        case 'd':
          return 3;
          break;

        default:
          return -1;
      }
    }
    else {return -1;}
  }

  void setSensorStatus(const char* device, bool status) {
    //On
    int index = getIndex(device);

    if (index != -1){
      //On
      if (status) {
        sensors |= (1 << index);  //Shift the 1 to the index, and use OR 
      } 
      //Off
      else {
        sensors &= ~(1 << index);  //Shift the 1 to the index, invert with not, and use AND
      }
    }
    
    

  }
  bool readStatus(const char* device) {
    int index = getIndex(device);

    if (index != -1){
      return (sensors >> index) & 1;  // Shift the n-th bit to the rightmost, then AND with 1
    }
    return 0;
    
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




const byte address =253;

char error[64] = "" ;
uint8_t errorLength = 0;



File file;

void setup() {
  
  
  //Setup Serial and LoRa
  Serial.begin(115200); //Rx Tx


  //Set up wire
  Wire.begin();

  delay(1000);

  GY85.init();


  delay(100);
  //if (SD.begin(10)) {
  //  file = SD.open("data.txt", FILE_WRITE);
  //  if (file) {
  //    debug.setSensorStatus("sd", true);
  //  }  
  //}
}

void RaiseError(char* message){
  //Get the error length and make sure it doesnt exeed limit
  errorLength = strlen(message);
  errorLength = (errorLength < 64) ? errorLength : 64;

  //Copy the message to the errorMessage to the error object
  memcpy(error, message, errorLength);
}

void ReadDebug(){
  debug.batteryVoltage = (1.1 * 1024.0) / analogRead(A0);
  extern int __heap_start, *__brkval;
  int freeRam = 0;

  extern int __heap_start,*__brkval;

  int v;

  freeRam = (int)&v - (__brkval == 0  ? (int)&__heap_start : (int) __brkval);  

  debug.memUsage = freeRam;

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
  debug.setSensorStatus("gy", true);
  
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
  debug.packetCount += 1;
  }


}



unsigned long time {};

float humidity {};
float temperature {};
int del = 300;
int debugCount = 0;

void loop() {
  if (dht_sensor.measure( &temperature, &humidity )) {
    debug.setSensorStatus("dht", true);
    data.temperature = temperature*100;
    data.humidity = humidity;
  } 
  
  for(int i = 0; i < Serial.available(); i++){
      gps.encode(Serial.read());
  }
  if (time + del > millis()){return;}

 
  if (gps.location.isValid())
  {  
    debug.setSensorStatus("gps", true);
    data.gps[0] = gps.location.lng() * pow(10, 6);
    data.gps[1] = gps.location.lat() * pow(10, 6);
    data.height = gps.altitude.isValid() ? gps.altitude.meters() : 0;
    
    data.velocity = gps.speed.isValid() ? gps.speed.mps()*100 : 0;
  }
  

  if (debugCount < 5){
    readSensors();
    BuildPacket(0);
    SendPacket();
    debugCount+=1;
    //Serial.println("LALA");


  }
  else{
    //Serial.println(1);
    ReadDebug();
    //Serial.println(2);
    BuildPacket(1);
    //Serial.println(3);
    SendPacket();
    //Serial.println(4);
    debugCount = 0;

  }
  
  //delay(30);
  time = millis();

}