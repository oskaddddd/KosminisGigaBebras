// Create a HardwareSerial instance for the LoRa module
HardwareSerial LoRa_serial(1);

// Define the pins for RX and TX
const byte rxPin = 16;
const byte txPin = 17;

// Define the LoRa channel and address (customize as needed)
const uint16_t address = 0x1234;  // Example address (0x1234)
const byte channel = 9;           // Example channel (0x09)

void setup() {
  // Initialize the Serial Monitor for debugging
  Serial.begin(9600);
  
  // Initialize rfSerial with correct parameters: baud rate, data format, rxPin, txPin
  LoRa_serial.begin(9600, SERIAL_8N1, rxPin, txPin); // 9600 baud, 8 data bits, no parity, 1 stop bit

  // Give some time for the LoRa module to initialize
  delay(1000);

  // Set the address
  uint8_t addressH = (address >> 8) & 0xFF;  // High byte of the address
  uint8_t addressL = address & 0xFF;         // Low byte of the address

  byte addressConfig[] = {0xC0, 0x00, 0x02, addressH, addressL};  // Address config command
  LoRa_serial.write(addressConfig, sizeof(addressConfig));
  delay(100);  // Allow time for the module to process the command

  // Set the channel
  byte channelConfig[] = {0xC0, 0x04, 0x01, channel};  // Channel config command
  LoRa_serial.write(channelConfig, sizeof(channelConfig));
  delay(100);  // Allow time for the module to process the command

  // Print a message to indicate the setup is complete
  Serial.println("LoRa setup complete");
}

void loop() {
  // Your loop code here (e.g., send or receive LoRa messages)
}