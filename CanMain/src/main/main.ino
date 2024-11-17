//Libraries
#include  <SD.h> 
#include  <SPI.h>
#include  <Arduino.h>
#include  <Wire.h>
#include  <SoftwareSerial.h> 
#include  <cstdint>

//Sensors
#include  <DHT11.h> 
#include  <MPU9250_asukiaaa.h> 
#include  <Adafruit_BMP280.h> 

//LoRa
#include  <E220.h>

#define m0 6
#define m1 7
#define aux 5

MPU9250_asukiaaa MPU_sensor;
dht11 DHT_sensor;
Adafruit_BMP280 BMP_sensor;

const int DHT_pin = 5;
const byte rxPin = 3;
const byte txPin = 2;


int16_t temperature, acceleration[3], angVelocity[3], magneticField[3];
uint16_t preasure, vocConc, co2Conc;
uint8_t humidity;
float gps[3];




SoftwareSerial LoRa(rxPin, txPin);
Stream &kgbBangos = (Stream &)LoRa;

#pragma pack(push, 1)  // Ensure no padding between members
struct Packet {
    uint8_t header;               // 1 byte
    uint8_t packetId;             // 1 byte
    uint8_t senderId;             // 1 byte
    float timestamp;              // 4 bytes
    int16_t angVelocity[3];          // 2 bytes
    int16_t acceleration[3];      // 6 bytes (3 * 2 bytes)
    int16_t magneticField[3];    // 6 bytes (3 * 2 bytes)
    float gps[3];                 // 12 bytes (3 * 4 bytes)
    int16_t temperature;          // 2 bytes
    uint16_t pressure;            // 2 bytes
    uint8_t humidity;             // 1 byte
    uint16_t vocConcentration;    // 2 bytes
    uint16_t co2Concentration;    // 2 bytes
};

#pragma pack(pop)

File file;

void setup() {
  //Setup Serial
  Serial.begin(9600);
  LoRa.begin(9600);


  if(!Wire.begin()){
    Serial.print("Failed to inicialize ");
  }
  //Wait for serial to begin
  while (!Serial);
  

  if (!SD.begin()) {
    Serial.println("SD card failed");
    return;
  }

  //Test if SD card is working
  file = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  if (!file) {
    Serial.println("SD card failed");
  }
  

  
  MPU_sensor.setWire(&Wire);
  MPU_sensor.beginAccel();
  MPU_sensor.beginGyro();
  MPU_sensor.beginMag();


  if (!BMP_sensor.begin()) {
    Serial.println("BMP280 failed");
  }

  pinMode(DHT_pin, INPUT);
  E220 radioModule(&kgbBangos, m0, m1, aux);

  //Wait for radio module to turn on
  while (!radioModule.init()) {
    delay(1000);
  }

  //Wait for radio module to set up
  uint8_t i = 0;
  while ((!radioModule.setAddress(230, true) || !radioModule.setChannel(123, true)) && i < 5){
    i++;
    delay(500);
  }

  //radioModule.printBoardParameters();
}

void readSensors() {

  float check;

  //Get accelerometer data
  if (MPU_sensor.accelUpdate() == 0) {
    acceleration[0] = MPU_sensor.accelX()*100;
    acceleration[1] = MPU_sensor.accelY()*100;
    acceleration[2] = MPU_sensor.accelZ()*100;
  }

  //Get gyro data
  if (MPU_sensor.gyroUpdate() == 0) {
    angVelocity[0] = MPU_sensor.gyroX()*100;
    angVelocity[1] = MPU_sensor.gyroY()*100;
    angVelocity[2] = MPU_sensor.gyroZ()*100;
  }

  //Get humidity data
  DHT_sensor.read(DHT_pin);
  humidity = DHT_sensor.humidity;

  //Read temperature (100*C) and check if its valid
  check = BMP_sensor.readTemperature();
  if (check == NAN){
    temperature = (DHT_sensor.temperature*100);
  }
  else{
    temperature = (check*100);
  }

  //Read preassure (Pa) and check if its valid
  check = BMP_sensor.readPressure();
  if (check == NAN){
    preasure = 0;
  }
  else{
    preasure = check;
  }

}



//Print data to file
template <typename... Args>
void writeToFile(Args... args) {
  (file.print(args), ..., file.print(" "));  // Print each argument followed by a space
  file.println();
}



//void packets(){
//
//    Packet packet = {};
//    
//    // Assign sensor values to the packet
//    packet.header = 0x01;            // Example header
//    packet.packetId = 0x02;          // Example packet ID
//    packet.senderId = 0x03;          // Example sender ID
//    packet.timestamp = 0.0f;         // Undefined, leave as default
//
//    // Sensor values
//    packet.angVelocity[0] = static_cast<int16_t>(gX * 100); // Example scaling to fit int16_t
//    packet.angVelocity[1] = static_cast<int16_t>(gY * 100);
//    packet.angVelocity[2] = static_cast<int16_t>(gZ * 100);
//
//    packet.acceleration[0] = static_cast<int16_t>(aX * 100); 
//    packet.acceleration[1] = static_cast<int16_t>(aY * 100); 
//    packet.acceleration[2] = static_cast<int16_t>(aZ * 100);
//
//    packet.magneticField[0] = static_cast<int16_t>(4); // Example magnetic field data
//    packet.magneticField[1] = static_cast<int16_t>(5);
//    packet.magneticField[2] = static_cast<int16_t>(6);
//
//    packet.gps[0] = 0.0f; // Undefined GPS values
//    packet.gps[1] = 0.0f;
//    packet.gps[2] = 0.0f;
//
//    packet.temperature = static_cast<int16_t>(temperature * 100); // Celsius to fixed-point
//    packet.pressure = static_cast<uint16_t>(pressure * 100); // hPa to fixed-point
//    packet.humidity = static_cast<uint8_t>(humidity); // Humidity in percentage
//
//    packet.vocConcentration = 0; // Undefined VOC value
//    packet.co2Concentration = 0; // Undefined CO2 value
//
//    // Serialize packet into byte array
//    uint8_t byteArray[sizeof(Packet)];
//    memcpy(byteArray, &packet, sizeof(Packet));
//    packets();
//  if(KosminesGigaBebroBangos.available()){
//    for (size_t i = 0; i < sizeof(Packet); ++i) {
//      KosminesGigaBebroBangos.print(byteArray[i], HEX);
//    }
//    Serial.println();
//  }
//}

void SendData(){
  if(kgbBangos.available()){
    kgbBangos.send();
    kgbBangos.flush();
  }
}

void loop() {
  readSensors();
  writeToFile();
  //packets();
  SendData();
  delay(250);
}