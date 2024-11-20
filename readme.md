# Packet structure

The packet is sent in byte form.

## Header

The header is used to identify what kind of packet this is and to know the time that it was sent.

```go
startByte: uint8  (1 byte) -> 0x00
lenght: uint8  (1 byte) -> The length of the packet in bytes
packetId: uint8  (1 byte) -> 0x00 for a data packet and 0x01 for a debug packet 0x01
senderId: uint8  (1 byte) -> used to varify the sender can be changed, curerently set to 0xE6
timestamp: uint32 (4 bytes) -> the millis, since the canSat was turned on
```
8 bytes

## Payload

The type of payload is identified by the `packetId` byte. The payload can be either **Data** or **Debug**.

### Data

All the collected data is sent using this packet. It will be sent multiple times per second.

```go
angVelocity: [int16 int16 int16] (6 bytes) -> 100 * °/s
acceleration: [int16 int16 int16] (6 bytes) -> 100 * g
magneticField: [int16 int16 int16] (6 bytes) -> 100 * mTesla
gps: [float32 float32 float32] (12 bytes)
temperature: int16 (2 bytes) -> 100 * °C
pressure: uint16 (2 bytes) -> Pa
humidity: uint8 (1 byte) -> %;
vocConcentration: uint16 (2 bytes) -> ppm
co2Concentration: uint16 (2 bytes) -> ppm
checkSum: uint8 (1 byte) -> calculated by using xor on all the bytes of the packet.
```
40 bytes (total 48 with header bytes)

### Debug

This is used to diagnose issues with the CanSat and see if everything is working correctly

```go
packetCount: uint16 (2 bytes) -> How many data packets were sent in total
batteryVoltage: uint16 (2 bytes) -> 100*V 
memUsage: uint16 (2 bytes) -> kB
sensors: uint8 (1 byte) -> Each bit represents the functioning of deffirent sensors. [0, 0, sdCard, co2, voc, dht11, gy91, Parashute]
errors: uint8 [dynamic] (X bytes) -> Up to 48 characters (bytes)
checksum: uint8 (1 byte) -> calculated by using xor on all the bytes of the packet.
```
8 to 56 bytes (total 64 with header bytes)


.

