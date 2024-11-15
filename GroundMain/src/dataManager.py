import struct
from sortedcontainers import SortedList
import logging
import serial
import threading

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
    def __init__(self, CanId:bytes = 0xE6):
        
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
            "packetId": self.b.uint8(packet, 0), #1 byte
            "senderId": self.b.uint8(packet, 1), #1 byte
            "timestamp": self.b.float32(packet, 2) #4 bytes
        }
        byteCount = 6

        #Check if the packet is valid
        if header['senderId'] != self.CanId:
            logging.warning(f"Wrong senderId: {header}, instead of {self.CanId}")
            return
        
        
        #Read
        if header['packetId'] == self.PacketTypes['data']:
            payload = {}
            payload['angVelocity'] = [self.b.int16(packet, byteCount+i*2)/100 for i in range(3)], #6 bytes
            byteCount+=6
            payload['acceleration']= [self.b.int16(packet, byteCount+i*2)/100 for i in range(3)], #6 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=6
            payload['magneticField']= [self.b.int16(packet, byteCount+i*2)/100 for i in range(3)], #6 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=6
            payload['gps']= [self.b.float32(packet, byteCount+i*4)/100 for i in range(3)], #12 bytes
            byteCount+=12
            payload['temprature']=self.b.int16(packet, byteCount)/100, #2 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=2
            payload['preasure']=self.b.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            payload['humidity']=self.b.uint8(packet, byteCount), #1 byte
            byteCount+=1
            payload['vocConcentration']=self.b.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            payload['co2Concentration']= self.b.uint16(packet, byteCount) #2 bytes
            byteCount+=2
            
            
        elif header['packetId'] == self.PacketTypes['debug']:
            payload = {}
            payload['packetCount'] = self.b.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            payload['baterryVoltage'] = self.b.uint16(packet, byteCount), #2 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=2
            payload['parashute'] = bool(self.b.uint8(packet, byteCount)), #1 byte
            byteCount+=1
            payload['memUsage'] = self.b.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            payload['gy91'] = bool(self.b.uint16(packet, byteCount)), #2 bytes
            byteCount+=2
            payload['dht11'] = bool(self.b.uint16(packet, byteCount)), #2 bytes
            byteCount+=2
            payload['voc'] = bool(self.b.uint16(packet, byteCount)), #2 bytes
            byteCount+=2
            payload['co2'] = bool(self.b.uint16(packet, byteCount)), #2 bytes
            byteCount+=2
            payload['sdCard'] = bool(self.b.uint16(packet, byteCount)), #2 bytes
            byteCount+=2
            




    #A function that adds data to the main DataStore
    def add_data(self, data: object):
        pass
    
    #A function that deals with debug data
    def debug_data(self, data):
        pass
        
        