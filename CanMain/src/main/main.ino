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
  uint8_t start[2] = {0x98, 0x5c}; 
  uint8_t senderId = 0xe6;         
  uint8_t length;
  uint8_t packetType;                           
  uint32_t timestamp;
  
  PacketHeader(){
    timestamp = millis();
  }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DataPayload {
<<<<<<< Updated upstream
  int16_t angVelocity[3] {};       // 2 bytes
  int16_t acceleration[3] {};      // 6 bytes (3 * 2 bytes)
  int16_t magneticField[3] {};     // 6 bytes (3 * 2 bytes)
  float gps[3];                 // 12 bytes (3 * 4 bytes)
  int16_t temperature {};          // 2 bytes
  //uint16_t pressure;            // 2 bytes
  //uint8_t humidity;             // 1 byte
  uint16_t vocConcentration {};    // 2 bytes
  uint16_t co2Concentration {};    // 2 bytes
=======
  int16_t angVelocity[3] {};       // 6 bytes | 2 bytes * 3 
  int16_t acceleration[3] {};      // 6 bytes | 2 bytes * 3 
  int16_t magneticField[3] {};     // 6 bytes | 2 bytes * 3 
  uint32_t gps[2] {};              // 8 bytes | 4 bytes * 2
  uint16_t height {};              // 2 bytes | 2 bytes * 1
  int16_t velocity {};             // 2 bytes | 2 bytes * 1
  int16_t temperature = 20;          // 2 bytes | 2 bytes * 1
  uint8_t humidity = 30;             // 1 bytes | 1 bytes * 1
  uint16_t co2 {};
};
#pragma pack(pop)

#pragma pack(push, 1) 
struct DebugPayload {
  uint16_t packetCount = 0;
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



<<<<<<< Updated upstream
=======
//Libraries, all of them can be found in the libs folder
#include  <SD.h> 
#include  <SPI.h>
#include  <Wire.h>

//Sensors
#include "GY_85.h"
#include "DHT_Async.h"
#include <TinyGPSPlus.h>
#include <MQ135.h>


//Setup the dht11
#define DHT_SENSOR_TYPE DHT_TYPE_11
static const uint8_t DHT_SENSOR_PIN = 2;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);


#define PIN_MQ135 A0
MQ135 CO2_sensor(PIN_MQ135);

const uint8_t photores_pin = A6; 
const uint8_t speaker_pin = 8;



TinyGPSPlus gps;
GY_85 GY85;



const byte rx_gps = 2;
const byte tx_gps = 3;
//const byte rx_lora = 5;
//const byte tx_lora = 4;


SoftwareSerial gps_serial(rx_gps, tx_gps);


const byte address =64;

char error[64] = "" ;
uint8_t errorLength = 0;



File file;

void setup() {
  

  //Data is sent to the radio over tx and the gps data is reciever over rx.
  //To upload code, remove the thingy connecting gps to the rx pin.
  //GPS can't use software serial cause it's simply too slow and drops data.
  Serial.begin(115200);

  //Set up wire
  Wire.begin();

  delay(1000);

  GY85.init();

<<<<<<< Updated upstream
  if (!SD.begin()) {
    Serial.println("SD card failed");
    return;
  }

  //Test if SD card is working
  file = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  if (!file) {
    Serial.println("SD card failed");
  }
=======

  delay(100);
  //if (SD.begin(10)) {
  //  file = SD.open("data.txt", FILE_WRITE);
  //  if (file) {
  //    debug.setSensorStatus("sd", true);
  //  }  
  //}

  
>>>>>>> Stashed changes

}

void RaiseError(char* message){
  //Get the error length and make sure it doesnt exeed limit
  errorLength = strlen(message);
  errorLength = (errorLength < 64) ? errorLength : 64;

  //Copy the message to the errorMessage to the error object
  memcpy(error, message, errorLength);
}

<<<<<<< Updated upstream
void ser_wait(int command_len){
  while (Serial.available() <= command_len){
    delay(100);
    Serial.print("waiting for commands {");Serial.print(command_len); Serial.print("} response");
=======
      //Mark SD as working and close the file
      debug.setSensorStatus("sd", true);
      file.close();
    }
    tone(speaker_pin, 200);
    
    
    
>>>>>>> Stashed changes
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


<<<<<<< Updated upstream
  delay(500);
  while(!Serial){delay(100);}

  return 1;
}
=======
//Low pass filter smoothing constant
const float lpAlpha = 0.5;
int avgAcc[3] {};
>>>>>>> Stashed changes



void readSensors() {

  int* accelerometerReadings = GY85.readFromAccelerometer();
  data.acceleration[0] = GY85.accelerometer_x(accelerometerReadings)*100;
  data.acceleration[1] = GY85.accelerometer_y(accelerometerReadings)*100;
  data.acceleration[2] = GY85.accelerometer_z(accelerometerReadings)*100;
  
  
  int* compassReadings = GY85.readFromCompass();
  data.angVelocity[0] = GY85.compass_x(compassReadings)*100;
  data.angVelocity[1] = GY85.compass_y(compassReadings)*100;
  data.angVelocity[2] = GY85.compass_z(compassReadings)*100;
 
  //Read compass data
  gyroReadings = GY85.readGyro();
  //Data multiplied by 100 to keep decimal
  data.angVelocity[0] = GY85.gyro_x(gyroReadings)*100;
  data.angVelocity[1] = GY85.gyro_y(gyroReadings)*100;
  data.angVelocity[2] = GY85.gyro_z(gyroReadings)*100;

  data.co2 = CO2_sensor.getCorrectedPPM(data.temperature, data.humidity);
  //Set the gy85 as working int he debug packet
  debug.setSensorStatus("gy", true);
>>>>>>> Stashed changes
  
}

void ReadDebug(){
  debug.batteryVoltage = (1.1 * 1024.0) / analogRead(A0);
  extern int __heap_start, *__brkval;
  int free_memory;
  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__heap_start);
  } else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  debug.memUsage = free_memory;

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
    errorLength = 0;(errorLength <= maxErrorLength) ? errorLength : maxErrorLength;

    packetLength = hSize + dSize + errorLength +fSize; 

    header.length = packetLength;

    //Copy data to the packet
    memcpy(Packet, &header, hSize);
    memcpy(Packet + hSize, &debug, dSize);
    //memcpy(Packet + hSize + dSize, &error, errorLength);

  }

  //calculate the checksum
  footer.CalcChecksum();
  
  memcpy(packetLength-fSize, &footer, fSize);

    
}

void SendPacket(){
<<<<<<< Updated upstream
  Serial.println("Sending");
  Serial.write(Packet, packetLength);

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
=======
  if (Serial.availableForWrite() >= packetLength){
    Serial.write(Packet, packetLength);
    memset(Packet, 0, sizeof(Packet));
    debug.packetCount+=1;
  }


}



unsigned long time {};

float humidity {};
float temperature {};
<<<<<<< Updated upstream
int del = 300;
=======

//Delay between packets
int del = 200;

//After how many normal packets should a debug packet be sent
uint8_t debugFreq = 5;
>>>>>>> Stashed changes
int debugCount = 0;

void loop() {

  if (dht_sensor.measure( &temperature, &humidity )) {
        debug.setSensorStatus("dht", true);
        data.temperature = temperature*100;
        data.humidity = humidity;
    }
  if (time + del > millis()){
    //Special treatment for the gps cause hes a very special boy
    
    if (gps.ready())
    { 
      debug.setSensorStatus("gps", true);  
      
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

  if (debugCount >= 5){
    //ReadDebug();
    //BuildPacket(1);
    //SendPacket();
    //debugCount = 0;
//
  }
  else{
    readSensors();
    BuildPacket(0);
    SendPacket();
    debugCount += 1;
  }
  
  //if (debug.readStatus("gps")){
  //  WriteToFile();
  //}
  

  delay(30);
  time = millis();

>>>>>>> Stashed changes
}