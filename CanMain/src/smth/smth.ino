#include  <E220.h>
#include  <SD.h>
#include  <SPI.h>
#include  <Arduino.h>
#include  <dht11.h>
#include  <MPU9250_askiaaa.h>
#include  <Adafruit_BMP280.h>
#include  <Wire.h>

const int DHTPIN = 5;
const byte rxPin = 3;
const byte txPin = 2;

File myFile;
MPU9250_asukiaaa mySensor;
dht11 DHT;
Adafruit_BMP280 bmp; // I2C
SoftwareSerial sender(rxPin, txPin);
E220 radioModule(&sender, 8, 9, 10);

float humidity, temperature, aX, aY, aZ, gX, gY, gZ, pressure;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Wire.begin();

  if (!SD.begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }

  myFile = SD.open("test.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("Error opening test.txt");
    return;
  }

  Serial.println("test.txt:");
  while (myFile.available()) {
    Serial.write(myFile.read());
  }
  myFile.close();

  // Initialize MPU9250
  Wire.begin();
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  // Initialize BMP280
  if (!bmp.begin()) {
    Serial.println("Could not find BMP280 sensor, check wiring!");
    while (1);
  }

  // Initialize DHT11
  pinMode(DHTPIN, INPUT);

  // Initialize E220 module
  sender.begin(9600); // Set the baud rate
  while (!radioModule.init()) {
    delay(5000);
  }
  radioModule.setAddress(0, true);
  Serial.println(radioModule.getAddress());
  Serial.println("Labas");
  radioModule.printBoardParameters();
}

void loop() {
  readSensors();
  writeToFile();
  delay(2000);
}

void readSensors() {
  // Read accelerometer data
  if (mySensor.accelUpdate() == 0) {
    aX = mySensor.accelX();
    aY = mySensor.accelY();
    aZ = mySensor.accelZ();
  }

  // Read gyro data
  if (mySensor.gyroUpdate() == 0) {
    gX = mySensor.gyroX();
    gY = mySensor.gyroY();
    gZ = mySensor.gyroZ();
  }

  // Read BMP280 data
  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0F; // hPa

  // Read DHT11 data
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