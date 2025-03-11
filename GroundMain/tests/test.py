from serial import Serial
from serial.tools import list_ports_l

ports = list_ports.comports(True)
print(ports)