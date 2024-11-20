//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>
#include  <HardwareSerial.h> 
#include  <cstdint>


//Sensors
#include  <dht11.h>
#include  <MPU9250_asukiaaa.h> 
#include  <Adafruit_BMP280.h> 

MPU9250_asukiaaa MPU_sensor;
dht11 DHT_sensor;
Adafruit_BMP280 BMP_sensor;

HardwareSerial rfSerial(1);


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
    data.acceleration[0] = MPU_sensor.accelX()*100;
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
  if(rfSerial.available()){
    rfSerial.write(Packet, packetLength);
  }
}

void loop() {

  //writeToFile();
  //packets();
  readSensors();
  BuildPacket(0);
  SendPacket();
  delay(250);
  BuildPacket(1);
  SendPacket();
}