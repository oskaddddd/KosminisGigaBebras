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
//e220 pins
#define m0 6
#define m1 7
#define aux 5
const byte rxPin = 3;
const byte txPin = 2;

const int DHTPIN = 5;

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
void setup() {
  Serial.begin(9600);
  sender.begin(9600);

  while (!Serial); //check for Serial
  Wire.begin();
  //check for SD
  if (!SD.begin()) {
    Serial.println("false");
    return;
  }
  //check for specified file in SD
  myFile = SD.open("KosminioGigaBebrofailai.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("false");
    return;
  }

  myFile.close();
  //sensor initialization
  mySensor.setWire(&Wire);//MPU9250
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  if (!bmp.begin()) {
    Serial.println("BMP280 not found");
    while (1);
  }

  pinMode(DHTPIN, INPUT);

  E220 radioModule(&KosminesGigaBebroBangos, m0, m1, aux);
  while (!radioModule.init()) {
    delay(5000); //waiting for radio module detection
  }

  radioModule.setAddress(230, true);
  radioModule.setChannel(123,123);
  Serial.println(radioModule.getAddress()); //check if the radio module isn't fried
  radioModule.printBoardParameters();
}

void readSensors() {
  //accelerometer
  if (mySensor.accelUpdate() == 0) {
    aX = mySensor.accelX();
    aY = mySensor.accelY();
    aZ = mySensor.accelZ();
  }
  //gyroscope
  if (mySensor.gyroUpdate() == 0) {
    gX = mySensor.gyroX();
    gY = mySensor.gyroY();
    gZ = mySensor.gyroZ();
  }

  int temperature1 = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0F;

  int chk = DHT.read(DHTPIN);
  humidity = DHT.humidity;
  int temperature2 = DHT.temperature;
  int temperature=(temperature1+temperature2)/2; //avrg of temperatures recieved from modules
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

    Packet packet = {};
    
    // Assign sensor values to the packet
    packet.header = 0x01;            // Example header
    packet.packetId = 0x02;          // Example packet ID
    packet.senderId = 0x03;          // Example sender ID
    packet.timestamp = 0.0f;         // Undefined, leave as default

    // Sensor values
    packet.angVelocity[0] = static_cast<int16_t>(gX * 100); // Example scaling to fit int16_t
    packet.angVelocity[1] = static_cast<int16_t>(gY * 100);
    packet.angVelocity[2] = static_cast<int16_t>(gZ * 100);

    packet.acceleration[0] = static_cast<int16_t>(aX * 100); 
    packet.acceleration[1] = static_cast<int16_t>(aY * 100); 
    packet.acceleration[2] = static_cast<int16_t>(aZ * 100);

    packet.magneticField[0] = static_cast<int16_t>(4); // Example magnetic field data
    packet.magneticField[1] = static_cast<int16_t>(5);
    packet.magneticField[2] = static_cast<int16_t>(6);

    packet.gps[0] = 0.0f; // Undefined GPS values
    packet.gps[1] = 0.0f;
    packet.gps[2] = 0.0f;

    packet.temperature = static_cast<int16_t>(temperature * 100); // Celsius to fixed-point
    packet.pressure = static_cast<uint16_t>(pressure * 100); // hPa to fixed-point
    packet.humidity = static_cast<uint8_t>(humidity); // Humidity in percentage

    packet.vocConcentration = 0; // Undefined VOC value
    packet.co2Concentration = 0; // Undefined CO2 value

    // Serialize packet into byte array
    uint8_t byteArray[sizeof(Packet)];
    memcpy(byteArray, &packet, sizeof(Packet));
    packets();
  if(KosminesGigaBebroBangos.available()){
    for (size_t i = 0; i < sizeof(Packet); ++i) {
        KosminesGigaBebroBangos.print(byteArray[i], HEX);
    }
    KosminesGigaBebroBangos.println();
    KosminesGigaBebroBangos.flush();
  }
}

void loop() {
  readSensors();
  writeToFile();
  packets();
  Sender();
  delay(250);
}