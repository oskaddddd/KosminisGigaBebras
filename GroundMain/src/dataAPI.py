import struct
from sortedcontainers import SortedList
import json 

from time import sleep
import logging

from serial import Serial
from serial.tools import list_ports as list_ports


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
    
    def bit(b: bytes, start: int, bit: int):
        return((b>>start*8+bit) & 1)
    
    def string(b: bytes, start: int, length: int):
        return b[start: length].decode('utf-8')
    

Unpack.int8()


class DataMain():
    def __init__(self, CanId:bytes = 0xE6):
        
        #Object where all of the collected data is stored
        #Packets are stored in reverse order, so the newest one would have the index of 0
        self.path = "./GroundMain/assets/"
        
        self.dictData = []
        self.dictDebug = []
        
        with open(self.path+"data.json", 'r') as f:
            self.dictData = json.load(f)
            self.DataBase = SortedList(self.dictData, key=lambda x: -x['timestamp'])
        with open(self.path+"debug.json", 'r') as f:
            self.dictDebug = json.load(f)
            self.DebugData = SortedList(self.dictDebug, key=lambda x: -x['timestamp'])
        

        
        self.CanId = CanId
        self.unpack = Unpack()
        self.PacketCount = 0
        
        
        
        self.PacketTypes = {
            'data':0x00,
            'debug':0x01
        }
    
    #A function that proccesses incoming packets from the Can 
    def parse_packet(self, packet:bytes):

        logging.debug(f"Started parsing packet nr{self.PacketCount+1}: {packet}")
        
        if (packet[0] != 0x00):
            logging.warning("Recieved packet with wrong starting byte")
            return
        elif (len(packet) < 15):
            logging.warning("Recieved packet with inadequete length:", len(packet))
            return  
        header = {
            "length": self.unpack.uint8(packet, 1), #1 byte 
            "packetId": self.unpack.uint8(packet, 2), #1 byte
            "senderId": self.unpack.uint8(packet, 3), #1 byte
            "timestamp": self.unpack.uint32(packet, 4) #4 bytes
        }
        
        byteCount = 8

        #Check if the packet is valid
        if header['senderId'] != self.CanId:
            logging.warning(f"Wrong senderId: {header['senderId']}, instead of {self.CanId}")
            return
        
        payload = {'timestamp': header['timestamp']}
        #Read
        if header['packetId'] == self.PacketTypes['data']:

            payload['angVelocity'] = [self.unpack.int16(packet, byteCount+i*2)/100 for i in range(3)], #6 bytes
            byteCount+=6
            payload['acceleration']= [self.unpack.int16(packet, byteCount+i*2)/100 for i in range(3)], #6 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=6
            payload['magneticField']= [self.unpack.int16(packet, byteCount+i*2)/100 for i in range(3)], #6 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=6
            payload['gps']= [self.unpack.float32(packet, byteCount+i*4)/100 for i in range(3)], #12 bytes
            byteCount+=12
            payload['temprature']=self.unpack.int16(packet, byteCount)/100, #2 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=2
            payload['preasure']=self.unpack.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            payload['humidity']=self.unpack.uint8(packet, byteCount), #1 byte
            byteCount+=1
            payload['vocConcentration']=self.unpack.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            payload['co2Concentration']= self.unpack.uint16(packet, byteCount) #2 bytes
            byteCount+=2
            
            self.add_data(payload)
            
            
        elif header['packetId'] == self.PacketTypes['debug']:

            payload['packetCount'] = self.unpack.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            
            payload['baterryVoltage'] = self.unpack.uint16(packet, byteCount), #2 bytes (Values were multiplied by 100 to keep the decimal)
            byteCount+=2
            
            payload['memUsage'] = self.unpack.uint16(packet, byteCount), #2 bytes
            byteCount+=2
            
            payload['gy91'] = bool(self.unpack.bit(packet, byteCount, 0)), 
            payload['dht11'] = bool(self.unpack.bit(packet, byteCount, 1)), 
            payload['voc'] = bool(self.unpack.bit(packet, byteCount, 2)), 
            payload['co2'] = bool(self.unpack.bit(packet, byteCount, 3)), 
            payload['sdCard'] = bool(self.unpack.bit(packet, byteCount, 4)),
            byteCount+=1
            #All above combined are 1 byte 
            
            payload['message'] = self.unpack.string(packet, byteCount, header['length'])
            byteCount+=(header['length']-byteCount)
            
            self.debug_data(payload)
            

    #A function that adds data to the main DataStore
    def add_data(self, data: dict):
        #Add the data to 
        self.DataBase.add(data)
        self.dictData.append(data)
        with open(self.path + 'data.json', 'w') as f:
            json.dump(self.dictData, f)
        
    
    #A function that deals with debug data
    def debug_data(self, data: dict):
        #Add the data to 
        self.DebugData.add(data)
        self.dictDebug.append(data)
        with open(self.path + 'debug.json', 'w') as f:
            json.dump(self.dictDebug, f)
            
def SerialSetup():
    #Find available ports and promt user to choose
    ports = list_ports.comports()
    print("\nChoose port:")
    for i, port in enumerate(ports):
        print(f"-{i}- {port.name} {port.description}")
        
    ser = Serial(ports[int(input('Choose:'))].name, 9600)
    
    #Wait till serial initialises
    i = 0
    while (not ser.is_open):
        logging.debug("Waiting for serial to begin:", i)
        i+=1
        sleep(1)
    
    logging.debug("Serial initialised!")
    
    
    #Do handshake with arduno
    i = 0
    while True:
        logging.debug("Sending handshake:", i)
        ser.write(0x69)
        ans = 0x00
        
        for x in range(3):
            logging.debug("Waiting for response:", x)
            
            if ser.in_waiting > 0:
                
                ans = ser.read(1)
                if ans == 0x69:
                    break
            sleep(1)
            
        if ans == 0x69:
            break
    logging.debug("Handshake finished")
    
    return ser
            

        