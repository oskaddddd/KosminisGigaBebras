#include  <E220.h>
#include  <SD.h>
#include  <SPI.h>
#include  <Arduino.h>
#include  <dht11.h>
#include  <MPU9250_asukiaaa.h>
#include  <Adafruit_BMP280.h>
#include  <Wire.h>
#include  <SoftwareSerial.h>
#include  <cstdint>
#define m0 6
#define m1 7
#define aux 5
const int DHTPIN = 5;
const byte rxPin = 3;
const byte txPin = 2;
float humidity, temperature, aX, aY, aZ, gX, gY, gZ, pressure;
File myFile;
MPU9250_asukiaaa mySensor;
dht11 DHT;
Adafruit_BMP280 bmp;
SoftwareSerial sender(rxPin, txPin);
Stream &KosminesGigaBebroBangos = (Stream &)sender;

#pragma pack(push, 1)  // Ensure no padding between members
struct Packet {
    uint8_t header;               // 1 byte
    uint8_t packetId;             // 1 byte
    uint8_t senderId;             // 1 byte
    float timestamp;              // 4 bytes
    int16_t angVelocity;          // 2 bytes
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
void setup() {
  Serial.begin(9600);
  sender.begin(9600);

  while (!Serial);
  Wire.begin();

  if (!SD.begin()) {
    Serial.println("false");
    return;
  }

  myFile = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("false");
    return;
  }

  myFile.close();

  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  if (!bmp.begin()) {
    Serial.println("Could not find BMP280 sensor, check wiring!");
    while (1);
  }

  pinMode(DHTPIN, INPUT);
  E220 radioModule(&KosminesGigaBebroBangos, m0, m1, aux);
  while (!radioModule.init()) {
    delay(5000);
  }
  radioModule.setAddress(230, true);
  radioModule.setChannel(123,123);
  Serial.println(radioModule.getAddress());
  radioModule.printBoardParameters();
}

void readSensors() {

  if (mySensor.accelUpdate() == 0) {
    aX = mySensor.accelX();
    aY = mySensor.accelY();
    aZ = mySensor.accelZ();
  }

  if (mySensor.gyroUpdate() == 0) {
    gX = mySensor.gyroX();
    gY = mySensor.gyroY();
    gZ = mySensor.gyroZ();
  }

  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0F;

  int chk = DHT.read(DHTPIN);
  humidity = DHT.humidity;
  temperature = DHT.temperature;
}

void writeToFile() {
  myFile = SD.open("test.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("Error opening test.txt");
    return;
  }

  myFile.print("Humidity (%): ");
  myFile.print(humidity);

  myFile.print("Temperature (C): ");
  myFile.print(temperature);

  myFile.print("accelX: ");
  myFile.print(aX);
  myFile.print("\taccelY: ");
  myFile.print(aY);
  myFile.print("\taccelZ: ");
  myFile.print(aZ);
  
  myFile.print("\tgX: ");
  myFile.print(gX);
  myFile.print("\tgY: ");
  myFile.print(gY);
  myFile.print("\tgZ: ");
  myFile.print(gZ);

  myFile.print("\tPressure (hPa): ");
  myFile.print(pressure);
  myFile.println();
  
  myFile.close();
}
void packets(){

    Packet packet = {
        0x01,             // header
        0x02,             // packetId
        0x03,             // senderId
        123.456f,         // timestamp
        10,               // angVelocity
        { 1, 2, 3 },      // acceleration
        { 4, 5, 6 },      // magneticField
        { 10.1f, 20.2f, 30.3f },  // gps
        2500,             // temperature
        1013,             // pressure
        60,               // humidity
        100,              // vocConcentration
        400               // co2Concentration
    };

    // Create a byte array to hold the packet data
    uint8_t byteArray[sizeof(Packet)];

    // Copy the data from the packet struct into the byte array
    //std::memcpy(byteArray, &packet, sizeof(Packet));

}
void Sender(){
  if(KosminesGigaBebroBangos.available()){
    KosminesGigaBebroBangos.print("20");
    KosminesGigaBebroBangos.flush();
  }
}

void loop() {
  readSensors();
  writeToFile();
  packets();
  Sender();
  delay(2000);
}