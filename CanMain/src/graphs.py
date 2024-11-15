import struct
import matplotlib.pyplot as plt

# Example binary data (replace this with actual data)
binary_data = b'\x01\x02\x03' + struct.pack('f', 123456.78) + struct.pack('h', 512) + \
    struct.pack('hhh', 1, 2, 3) + struct.pack('hhh', 4, 5, 6) + \
    struct.pack('fff', 1.1, 2.2, 3.3) + struct.pack('h', 2500) + \
    struct.pack('H', 1013) + struct.pack('B', 45) + struct.pack('H', 300) + struct.pack('H', 400)

# Data structures
HEADER_SIZE = 1
PACKET_ID_SIZE = 1
SENDER_ID_SIZE = 1
TIMESTAMP_SIZE = 4
ANG_VELOCITY_SIZE = 2
ACCELERATION_SIZE = 6
MAGNETIC_FIELD_SIZE = 6
GPS_SIZE = 12
TEMPERATURE_SIZE = 2
PRESSURE_SIZE = 2
HUMIDITY_SIZE = 1
VOC_CONCENTRATION_SIZE = 2
CO2_CONCENTRATION_SIZE = 2

RECORD_SIZE = (HEADER_SIZE + PACKET_ID_SIZE + SENDER_ID_SIZE + TIMESTAMP_SIZE +
               ANG_VELOCITY_SIZE + ACCELERATION_SIZE + MAGNETIC_FIELD_SIZE + GPS_SIZE +
               TEMPERATURE_SIZE + PRESSURE_SIZE + HUMIDITY_SIZE + VOC_CONCENTRATION_SIZE +
               CO2_CONCENTRATION_SIZE)

# Parsing the binary data
timestamps = []
temperatures = []
pressures = []
humidities = []

offset = 0
while offset < len(binary_data):
    header, packet_id, sender_id = struct.unpack_from('BBB', binary_data, offset)
    offset += 3
    timestamp, = struct.unpack_from('f', binary_data, offset)
    offset += 4
    ang_velocity, = struct.unpack_from('h', binary_data, offset)
    offset += 2
    acceleration = struct.unpack_from('hhh', binary_data, offset)
    offset += 6
    magnetic_field = struct.unpack_from('hhh', binary_data, offset)
    offset += 6
    gps = struct.unpack_from('fff', binary_data, offset)
    offset += 12
    temperature, = struct.unpack_from('h', binary_data, offset)
    offset += 2
    pressure, = struct.unpack_from('H', binary_data, offset)
    offset += 2
    humidity, = struct.unpack_from('B', binary_data, offset)
    offset += 1
    voc_concentration, co2_concentration = struct.unpack_from('HH', binary_data, offset)
    offset += 4

    # Store data
    timestamps.append(timestamp)
    temperatures.append(temperature / 100.0)  # Scale down temperature
    pressures.append(pressure)
    humidities.append(humidity)

# Plotting the data
plt.figure(figsize=(12, 6))

# Temperature plot
plt.subplot(3, 1, 1)
plt.plot(timestamps, temperatures, label='Temperature (°C)', color='r')
plt.xlabel('Timestamp (s)')
plt.ylabel('Temperature (°C)')
plt.title('Temperature Over Time')
plt.grid(True)
plt.legend()

# Pressure plot
plt.subplot(3, 1, 2)
plt.plot(timestamps, pressures, label='Pressure (hPa)', color='b')
plt.xlabel('Timestamp (s)')
plt.ylabel('Pressure (hPa)')
plt.title('Pressure Over Time')
plt.grid(True)
plt.legend()

# Humidity plot
plt.subplot(3, 1, 3)
plt.plot(timestamps, humidities, label='Humidity (%)', color='g')
plt.xlabel('Timestamp (s)')
plt.ylabel('Humidity (%)')
plt.title('Humidity Over Time')
plt.grid(True)
plt.legend()

plt.tight_layout()
plt.show()
