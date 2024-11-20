//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>
#include  <HardwareSerial.h> 
#include  <cstdint>

//Sensors
#include  <DHT11.h> 
#include  <MPU9250_asukiaaa.h> 
#include  <Adafruit_BMP280.h> 

MPU9250_asukiaaa MPU_sensor;
dht11 DHT_sensor;
Adafruit_BMP280 BMP_sensor;

HardwareSerial rfSerial(1);


const byte rxPin = 16;
const byte txPin = 17;

const byte channel = 230;
const byte address[2] = {123, 123};


int16_t data.temperature, data.acceleration[3], data.angVelocity[3], data.magneticField[3];
uint16_t data.pressure, vocConc, co2Conc;
uint8_t data.humidity;
float data.gps[3];



File file;

#pragma pack(push, 1) 
struct PacketHeader {
  uint8_t startByte = 0x00;          // 1 byte
  uint8_t packetId;           // 1 byte
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
  char errors;

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

DataPacket data = {};

uint8_t Packet[64] = {}

void setup() {
  
  
  //Setup Serial and LoRa
  Serial.begin(9600);
  rfSerial.begin(9600, SERIAL_8N1, rxPin, txPin); //Rx Tx

  delay(1000);

  //Set the channel and the address of the LoRa
  uint8_t addressConfig[] = {0xC1, 0x00, 0x02, 0xC0, address[0], address[1]}; 
  uint8_t channelConfig[] = {0xC1, 0x04, 0x01, channel};

  rfSerial.write(addressConfig, sizeof(addressConfig));
  delay(100);
  rfSerial.write(channelConfig, sizeof(channelConfig));
4

  //Set up wire
  if(!Wire.begin()){
    Serial.print("Failed to inicialize ");
  }

  if (!SD.begin()) {
    Serial.println("SD card failed");
    return;
  }
  
  //Set up MPU
  MPU_sensor.setWire(&Wire);
  MPU_sensor.beginAccel();
  MPU_sensor.beginGyro();
  MPU_sensor.beginMag();

  //Setup DHT
  pinMode(DHT_pin, INPUT);


  //Test is BMP is working
  if (!BMP_sensor.begin()) {
    Serial.println("BMP280 failed");
  }
  //Test if SD card is working
  file = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  if (!file) {
    Serial.println("SD card failed");
  }

}


void readSensors() {

  float check;

  //Get accelerometer data
  if (MPU_sensor.accelUpdate() == 0) {
    data.acceleration[0] = [0] = MPU_sensor.accelX()*100;
    data.acceleration[1] = MPU_sensor.accelY()*100;
    data.acceleration[2] = MPU_sensor.accelZ()*100;
  }

  //Get gyro data
  if (MPU_sensor.gyroUpdate() == 0) {
    data.angVelocity[0] = MPU_sensor.gyroX()*100;
    data.angVelocity[1] = MPU_sensor.gyroY()*100;
    data.angVelocity[2] = MPU_sensor.gyroZ()*100;
  }

  //Get data.humidity data
  DHT_sensor.read(DHT_pin);
  data.humidity = DHT_sensor.data.humidity;

  //Read data.temperature (100*C) and check if its valid
  check = BMP_sensor.readdata.Temperature();
  if (check == NAN){
    data.temperature = (DHT_sensor.data.temperature*100);
  }
  else{
    data.temperature = (check*100);
  }

  //Read preassure (Pa) and check if its valid
  check = BMP_sensor.readdata.Pressure();
  if (check == NAN){
    data.pressure = 0;
  }
  else{
    data.pressure = check;
  }

}

//Print data to file
template <typename... Args>
void writeToFile(Args... args) {
  (file.print(args), ..., file.print(" "));  // Print each argument followed by a space
  file.println();
}


void Packet(uint8_t type){
  PacketHeader header;

  //Data Packet
  if (type == 0){
    header.packetId = 0x00;
    
    memcpy(packet, &header, sizeof(PacketHeader));
    memcpy(packet + sizeof(PacketHeader), &data, sizeof(data));
  }
  
}

//void SendData(){
//  if(kgbBangos.available()){
//    kgbBangos.send();
//    kgbBangos.flush();
//  }
//}

void loop() {
  readSensors();
  writeToFile();
  //packets();
  SendData();
  delay(250);
}