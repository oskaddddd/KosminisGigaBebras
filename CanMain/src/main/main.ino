#include  <Arduino.h>


//PACKET STRUCTS
uint8_t packetLength = 0;
uint8_t Packet[64] = {};

//Stuct for the packet header
#pragma pack(push, 1) 
struct PacketHeader {
  uint8_t start[2] = {0x98, 0x98}; 
  uint8_t senderId = 0xff;         
  uint8_t length {};
  uint8_t packetType {};                           
  uint32_t timestamp {};
  
  //Calculates the timestamp, when struct initialised
  PacketHeader(){
    timestamp = millis();
  }
};
#pragma pack(pop)

//Stuct for the data packet's payload
#pragma pack(push, 1)
struct DataPayload {
  int16_t angVelocity[3] {};       // 6 bytes | 2 bytes * 3 
  int16_t acceleration[3] {};      // 6 bytes | 2 bytes * 3 
  int16_t magneticField[3] {};     // 6 bytes | 2 bytes * 3 
  uint32_t gps[2] {};              // 8 bytes | 4 bytes * 2
  uint16_t height {};              // 2 bytes | 2 bytes * 1
  int16_t velocity {};             // 2 bytes | 2 bytes * 1
  int16_t temperature = 20;          // 2 bytes | 2 bytes * 1
  uint8_t humidity = 30;             // 1 bytes | 1 bytes * 1
  uint16_t co2 {};           // 1 bytes | 1 bytes * 1
};
#pragma pack(pop)

//Stuct for the debug packet's payload
#pragma pack(push, 1) 
struct DebugPayload {
  uint16_t packetCount = 1;        // 1 byte
  uint16_t batteryVoltage {};      // 4 bytes
  uint16_t memUsage {};            // 2 bytes
  uint8_t sensors {};              // 1 byte (sensor status stored in individual bits)
  uint16_t photoresistor {};       // 2 bytes

  //Get the bit index of a sensor 
  int getIndex(const char* device){
    
    if (device && *device){
      //Swich statement checks for the last letter of sensor's name, since it can't procces strings, but a single char is just a uint8
      switch (device[strlen(device)-1])
      {
        //gY
        case 'y': return 0; break;
        //dhT
        case 't': return 1; break;
        //gpS
        case 's': return 2; break;
        //sD
        case 'd': return 3; break;
        //co2
        case '2': return 4; break;

        default: return -1;
      }
    }
    else {return -1;}
  }
  //Function to update the sensor status. Input is a name of a sensor.
  void setSensorStatus(const char* device, bool status) {
    //Get the but index of a sensor by its name
    int index = getIndex(device);

    if (index != -1){
      //Change the bit at the index to:
      
      if (status) { //True
        sensors |= (1 << index);  //Shift the 1 to the index, and use OR 
      } 
      else { //False
        sensors &= ~(1 << index);  //Shift the 1 to the index, invert with not, and use AND
      }
    }
  }

  //Funciton to get the status of a sensor
  bool readStatus(const char* device) {
    //Get the but index of a sensor by its name
    int index = getIndex(device);

    if (index != -1){
      return (sensors >> index) & 1;  //Shift the n-th bit to the rightmost, then AND with 1
    }
    return 0;
  }
};
#pragma pack(pop)

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

//Where all the data is stored
DataPayload data;
DebugPayload debug;



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
static const int DHT_SENSOR_PIN = 6;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

#define PIN_MQ135 A0

MQ135 CO2_sensor(PIN_MQ135);



const int photores_pin = A6;  

TinyGPSPlus gps;
GY_85 GY85;

File file;

void setup() {

  //Data is sent to the radio over tx and the gps data is reciever over rx.
  //To upload code, remove the thingy connecting gps to the rx pin.
  //GPS can't use software serial cause it's simply too slow and drops data.
  Serial.begin(115200);

  //Set up wire
  Wire.begin();

  delay(1000);

  //Setup GY85
  GY85.init();
  delay(100);

  //Setups photoeresistor
  pinMode(photores_pin, INPUT);

  //Setup the SD card
  if (SD.begin(10)) {

    //Open the data file
    File file = SD.open("duom.txt", FILE_WRITE);

    if (file){
      //Mark the start of a new session
      file.println("--------");

      //Mark SD as working and close the file
      debug.setSensorStatus("sd", true);
      file.close();
    }
    
    
  }
}



//Check a couple values for the debug packet
void ReadDebug(){
  //Voltage detection DOESN'T work
  debug.batteryVoltage = (1.1 * 1023.0*100) / analogRead(A0);

  //Stolen code to get the ram usage
  extern int __heap_start,*__brkval;
  int v;

  debug.memUsage = (int)&v - (__brkval == 0  ? (int)&__heap_start : (int) __brkval);  

}


//Low pass filter smoothing constant
const float lpAlpha = 0.7;
int avgAcc[3] {};

//Pointers to the GY85 data
int* acc2 {};
int* acc1 {};
int* compassReadings {};
float* gyroReadings {};


void readSensors() {

  //Read 2 acceleration values and get the avarage
  acc1 = GY85.readFromAccelerometer();
  delay(50);
  acc2 = GY85.readFromAccelerometer();

  avgAcc[0] = (GY85.accelerometer_x(acc1) + GY85.accelerometer_x(acc2))/2;
  avgAcc[1] = (GY85.accelerometer_y(acc1) + GY85.accelerometer_y(acc2))/2;
  avgAcc[2] = (GY85.accelerometer_z(acc1) + GY85.accelerometer_z(acc2))/2;

  //If its the first time reading acceleration data
  if (!data.acceleration){
    //Data is multipled by 100 to keep the decimal
    data.acceleration[0] = avgAcc[0]*100;
    data.acceleration[1] = avgAcc[1]*100;
    data.acceleration[2] = avgAcc[2]*100;  
  }
  //If not the first time, apply low pass filter
  else{
    //Data is multipled by 100 to keep the decimal
    data.acceleration[0] = data.acceleration[0]*lpAlpha+(avgAcc[0]*(1.0-lpAlpha)*100);
    data.acceleration[1] = data.acceleration[1]*lpAlpha+(avgAcc[1]*(1.0-lpAlpha)*100);
    data.acceleration[2] = data.acceleration[2]*lpAlpha+(avgAcc[2]*(1.0-lpAlpha)*100);
  }
  
  //Read compass data
  compassReadings = GY85.readFromCompass();
  //Data multiplied by 100 to keep decimal
  data.magneticField[0] = GY85.compass_x(compassReadings)*100;
  data.magneticField[1] = GY85.compass_y(compassReadings)*100;
  data.magneticField[2] = GY85.compass_z(compassReadings)*100;
 
  //Read compass data
  gyroReadings = GY85.readGyro();
  //Data multiplied by 100 to keep decimal
  data.angVelocity[0] = GY85.gyro_x(gyroReadings)*100;
  data.angVelocity[1] = GY85.gyro_y(gyroReadings)*100;
  data.angVelocity[2] = GY85.gyro_z(gyroReadings)*100;

  data.co2 = CO2_sensor.getCorrectedPPM(data.temperature, data.humidity);

  //Set the gy85 as working int he debug packet
  debug.setSensorStatus("gy", true);
  debug.setSensorStatus("co2", true);
}

//Write data to file
void WriteToFile() {
  //Attempt to open file
  file = SD.open("duom.txt", FILE_WRITE);
  if (file) {
    //Write data to file and set status as working
    file.write(Packet, packetLength);
    debug.setSensorStatus("sd", true);
  }  
  //Set status as not working
  else{debug.setSensorStatus("sd", false);}
  file.close();
}

//Building the full packet
void BuildPacket(uint8_t type){
  //Initialise header and footer
  PacketHeader header;
  PacketFooter footer;

  //Sizes of footer, header and payload 
  uint8_t fSize = sizeof(footer);
  uint8_t hSize = sizeof(header);
  uint8_t dSize = 0;

  switch (type)
  {
    //Data packet
    case 0:
      //Set packet type
      header.packetType = 0x00;

      dSize = sizeof(data);

      //Calculate the length of the packet
      packetLength = hSize + dSize+fSize;

      header.length = packetLength;

      //Copy data to the packet
      memcpy(Packet, &header, hSize);
      memcpy(Packet + hSize, &data, dSize);
      break;

    //Debug packet
    case 1:
      //Set packet type
      header.packetType = 0x01;

      dSize = sizeof(debug);



      //Calculate the length of the packet
      packetLength = hSize + dSize +fSize; 
      header.length = packetLength;

      //Copy data to the packet
      memcpy(Packet, &header, hSize);
      memcpy(Packet + hSize, &debug, dSize);
      break;

    default: return; break;
  }


  //Calculate the checksum
  footer.CalcChecksum();
  
  //Copy the footer to the packet
  memcpy(Packet+(packetLength-fSize), &footer, fSize);    
}

//Send the packet
void SendPacket(){
  //Transmit the packet
  if (debug.photoresistor > 100){
    if (Serial.availableForWrite() >= packetLength){
      Serial.write(Packet, packetLength);

      //Add one to the packet count only if the packet is acutually sent
      debug.packetCount += 1;
    }
  }
  
  //Write the packet to SD
  WriteToFile();

  //Clear the packet buffer
  memset(Packet, 0, sizeof(Packet));

  


}



unsigned long time {};

//DHT lib needs pointer to floats and im too lazy to modify it to accept ints
float humidity {};
float temperature {};

//Delay between packets
int del = 500;

//After how many normal packets should a debug packet be sent
uint8_t debugFreq = 5;
int debugCount = 0;

void loop() {
  //Check dht sensor for new data
  if (dht_sensor.measure( &temperature, &humidity )) {
    //Set teh sensor's status to working 
    debug.setSensorStatus("dht", true);

    //Set the sensor's values
    data.temperature = temperature*100;
    data.humidity = humidity;
  } 
  
  //Read new gps packets
  for(int i = 0; i < Serial.available(); i++){
      gps.encode(Serial.read());
  }

  //Restart loop if its not yet time to send packets. 
  //Everything above needs to run with no delay, everything below runs according to del variable
  if (time + del > millis()){return;}

  //Check photoresistor value
  debug.photoresistor = analogRead(photores_pin);
  //Get new gps data
  if (gps.location.isValid())
  {  
    debug.setSensorStatus("gps", true);
    data.gps[0] = gps.location.lng() * pow(10, 6);
    data.gps[1] = gps.location.lat() * pow(10, 6);
    data.height = gps.altitude.isValid() ? gps.altitude.meters() : 0;
    
    data.velocity = gps.speed.isValid() ? gps.speed.mps()*100 : 0;
  }
  
  //Send Data Packet
  if (debugCount < debugFreq){
    readSensors();
    BuildPacket(0);
    SendPacket();
    debugCount+=1;
  }
  //Send debug packet
  else{
    ReadDebug();
    BuildPacket(1);
    SendPacket();
    debugCount = 0;
  }
  //Reset the delay
  time = millis();

}
