# About
This was a CanSat project developped by a team of students in VGTU inžinerijos licėjus.
![2025-05-19-21-49-26-340](https://github.com/user-attachments/assets/70e8b574-33f2-4834-ae80-0daaec59d9a9)
![Uploading Screenshot from 2025-12-03 14-14-43.png…]()


# Packet structure

The packet is sent in byte form.

## Header


The header is used to identify what kind of packet this is and to know the time that it was sent.

```go
startByte: uint8    (1 byte)  -> 0x98
senderId: uint8     (1 byte)  -> 0x98
lenght: uint8       (1 byte)  -> The length of the packet in bytes
packetType: uint8   (1 byte)  -> 0x00 data packet and 0x01 debug packet
timestamp: uint32   (4 bytes) -> the millis, since the canSat was turned on
```
8 bytes

## Payload

The type of payload is identified by the `packetId` byte. The payload can be either **Data** or **Debug**.

### Data

All the collected data is sent using this packet. It will be sent multiple times per second.

```go
angVelocity: [int16 int16 int16]    (6 bytes) -> 100 * °/s
acceleration: [int16 int16 int16]   (6 bytes) -> 100 * g
magneticField: [int16 int16 int16]  (6 bytes) -> 100 * mTesla
gps: [uint32 uint32]                (8 bytes) -> deg*10^6
height: uint16                      (2 bytes) -> m
velocity: int16                     (2 bytes) -> 100 * m/s
temperature: int16                  (2 bytes) -> 100 * °C
humidity: uint8                     (1 byte)  -> %
```
40 bytes (total 48 with header bytes)

### Debug

This is used to diagnose issues with the CanSat and see if everything is working correctly

```go
packetCount: uint16     (2 bytes) -> How many data packets were sent in total
batteryVoltage: uint16  (2 bytes) -> 100*V 
memUsage: uint16        (2 bytes) -> kB
sensors: uint8          (1 byte)  -> Each bit represents the functioning of deffirent sensors. [0, 0, 0, 0, sd, gps, dht, gy]
photoresistor: uint16   (2 bytes) -> 0 to 1024
checksum: uint8         (1 byte)  -> calculated by using xor on all the bytes of the packet.
```



.

