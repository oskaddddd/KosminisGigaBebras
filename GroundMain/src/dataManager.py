import struct
from sortedcontainers import SortedList
import logging

logging.basicConfig(level=logging.DEBUG)

#A class to parse bytes into data based on type
class Unpack():
    def int8(self, b: bytes, start: int):
        return struct.unpack('b', b[start:start+1])[0]

    def int16(b: bytes, start: int):
        return struct.unpack('h', b[start:start+2])[0]

    def int32(b: bytes, start: int):
        return struct.unpack('i', b[start:start+4])[0]

    def uint8(b: bytes, start: int):
        return struct.unpack('B', b[start:start+1])[0]

    def uint16(b: bytes, start: int):
        return struct.unpack('H', b[start:start+2])[0]

    def uint32(b: bytes, start: int):
        return struct.unpack('I', b[start:start+4])[0]

    def float32(b: bytes, start: int):
        return struct.unpack('f', b[start:start+4])[0]
    


class DataMain():
    def __init__(self, CanId:bytes = 0x9f):
        
        #Object where all of the collected data is stored
        self.DataStore = SortedList()
        self.CanId = CanId
        self.b = Unpack()
        self.PacketCount = 0
        
        self.PacketTypes = {
            'data':0x00,
            'debug':0x01
        }
    
    #A function that proccesses incoming packets from the Can 
    def parse_packet(self, packet:bytes):
        
        logging.debug(f"Started parsing packet nr{self.PacketCount+1}: {packet}")
        
        header = {
            "header": self.b.uint8(packet, 0), #1 byte
            "packetId": self.b.uint8(packet, 1), #1 byte
            "senderId": self.b.uint8(packet, 2), #1 byte
            "timestamp": self.b.float32(packet, 3) #4 bytes
        }
        
        #Check if the packet is valid
        if header['senderId'] != self.CanId:
            logging.warning(f"Wrong senderId: {header}, instead of {self.CanId}")
            return
        
        if header['packetId'] == self.PacketTypes['data']:
            payload = {
                'angVelocity': self.b.int16(packet, 7), #2 bytes
                'acceleration': [self.b.int16(packet, 9+i*2)/100 for i in range(3)], #6 bytes (Values were multiplied by 100 to keep the decimal)
                'magneticField': [self.b.int16(packet, 15+i*2)/100 for i in range(3)], #6 bytes (Values were multiplied by 100 to keep the decimal)
                'gps': [self.b.float32(packet, 21+i*4)/100 for i in range(3)], #12 bytes
                'temprature': None,
                'preasure': None,
                'humidity': None,
                'vocConcentration': None,
                'co2Concentration':None
                
            }
        elif header['packetId'] == self.PacketTypes['debug']:
            pass
            


    
    #A function that adds data to the main DataStore
    def add_data(self, data: object):
        pass
    
    #A function that deals with debug data
    def debug_data(self, data):
        pass
        
        