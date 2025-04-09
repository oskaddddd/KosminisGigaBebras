import struct
from sortedcontainers import SortedList
import json 

from time import sleep
import logging

from serial import Serial
from serial.tools import list_ports as list_ports

from operator import itemgetter

import numpy as np

logging.basicConfig(level=logging.DEBUG)

#A class to parse bytes into data based on type
class Unpack():
    def int8(self, b: bytes, start: int):
        return struct.unpack('b', b[start:start+1])[0]

    def int16(self, b: bytes, start: int):
        return struct.unpack('h', b[start:start+2])[0]

    def int32(self, b: bytes, start: int):
        return struct.unpack('i', b[start:start+4])[0]

    def uint8(self, b: bytes, start: int):
        return struct.unpack('B', b[start:start+1])[0]

    def uint16( self,b: bytes, start: int):
        return struct.unpack('H', b[start:start+2])[0]

    def uint32(self, b: bytes, start: int):
        return struct.unpack('I', b[start:start+4])[0]

    def float32(self, b: bytes, start: int):
        return struct.unpack('f', b[start:start+4])[0]
    
    def double(self, b: bytes, start: int):
        return struct.unpack('d', b[start:start+8])[0]
    
    def long(self, b: bytes, start: int):
        return struct.unpack('l', b[start:start+4])[0]
    
    def bit(self, b: bytes, start: int, bit: int):
        return((b[start]>>bit) & 1)
    
    def string(self, b: bytes, start: int, length: int):
        return b[start: length].decode('utf-8')
    



class DataMain():
    def __init__(self, startBytes: dict, canID:int):
        
        #Object where all of the collected data is stored
        #Packets are stored in reverse order, so the newest one would have the index of 0
        self.path = "./GroundMain/assets/"
        
        self.dictData = []
        self.dictDebug = []
        
        
        with open(self.path+"data.json", 'r') as f:
            self.dictData = json.load(f)
            if len(self.dictData) != 0:
                if input("There is data in data.json, clear to delete? y/n:") == 'y':
                    self.dictData = []
                
        self.DataBase = SortedList(self.dictData, key=lambda x: -x['timestamp'])
        with open(self.path+"debug.json", 'r') as f:
            self.dictDebug = json.load(f)
            if len(self.dictDebug) != 0:
                if input("There is data in debug.json, clear to delete? y/n:") == 'y':
                    self.dictDebug = []
        self.DebugData = SortedList(self.dictDebug, key=lambda x: -x['timestamp'])
        

        
        
        self.unpack = Unpack()
        self.packetCount = 0
        
        self.packetBuffer = bytearray()
        
        
        
        
        self.PacketTypes = {
            "data":0x00,
            "debug":0x01
        }
        
        self.canID = canID
        self.startBytes = startBytes
        
  
        
        self.footerLength = 3
        
    def parse_data(self, byteSnipet:bytes):
        self.packetBuffer.extend(byteSnipet)
        logging.debug(self.packetBuffer, "packetbuffer")
        #Check the snippet and 2 earlier bytes for a Header Id
        index = self.packetBuffer[len(self.packetBuffer)-len(byteSnipet)-(len(self.id)-1):].find(self.id)
        
        if index != -1:
            #Check if that bytes before the startID is the end seq
            if self.packetBuffer[index-2:index] == b"\r\n":
                #Send finished packet for parsing 
                packet = self.packetBuffer[:index]
                
                #Check if this packet is fine, else throw it in the trash 
                if packet.startswith(self.id):
                    #Finally parse the packet
                    return self.parse_packet(packet)
                else:
                    logging.debug(f"Packet was fucked up 1:{packet}")
                #Set the buffer to only store the new packet
                self.packetBuffer = self.packetBuffer[index:]
            else:
                logging.debug(f"Packet was fucked up 2:{packet}")
                #Woooooo weee wooo weeeeeee
                #SOMETHING IS MAJORLY FUCKED UP
                
                #implement fix later
#
        
    
    #A function that proccesses incoming packets from the Can 
    def parse_packet(self, packet:bytes) -> dict: 

        logging.debug(f"Started parsing packet nr{self.packetCount+1}: {packet.hex()}")
        
        if (len(packet) < 15):
            logging.warning("Recieved packet with inadequete length:", len(packet))
            return  
        header = {
            "start": [self.unpack.uint8(packet, (0+i)) for i in range(2)], #2 bytes
            "senderId": self.unpack.uint8(packet, 2), #1 byte
            "length": self.unpack.uint8(packet, 3), #1 byte 
            "packetType": packet[4], #1 byte
            "timestamp": self.unpack.uint32(packet, 5) #4 bytes
        }
        if len(packet) != header['length']:
            logging.warning("lengths dont match", packet)
            return 
        if header["start"] != self.startBytes or header['senderId'] != self.canID:
            logging.warning("Packet with wrong startbytes", packet, header['start'], self.startBytes, header["senderId"], self.canID)
            return 
        
        #Check checksum
        checksum = 0
        #Loop trough all the bytes up untill the footer
        for byte in packet[:header["length"]-self.footerLength]:
            checksum^=byte
        #Check if Chaksums match
        if checksum != packet[header['length']-self.footerLength]:
            logging.warning("Packet corrupted")
            return
            
        print(header['length'])
        byteCount = 9

        self.packetCount += 1
        
        payload = {'timestamp': header['timestamp']}
        
        match header['packetType']:
            case 0x00:
                #print(packet[byteCount:byteCount+6])
                payload['angVelocity'] = [self.unpack.int16(packet, byteCount+i*2)/100 for i in range(3)] #6 bytes
                byteCount+=6
                #print(packet[byteCount:byteCount+6])
                payload['acceleration']= [self.unpack.int16(packet, byteCount+i*2)/100/256 for i in range(3)] #6 bytes (Values were multiplied by 100 to keep the decimal)
                byteCount+=6
                #print(packet[byteCount:byteCount+6])
                payload['magneticField']= [self.unpack.int16(packet, byteCount+i*2)/100*0.92 for i in range(3)] #6 bytes (Values were multiplied by 100 to keep the decimal)
                byteCount+=6
                #print(packet[byteCount:byteCount+8])
                payload['gps']= [self.unpack.uint32(packet, byteCount+i*4) for i in range(2)] #12 bytes
                byteCount+=8
                #print(packet[byteCount:byteCount+2])
                payload['height'] = self.unpack.uint16(packet, byteCount)
                byteCount += 2
                #print(packet[byteCount:byteCount+2])
                payload['velocity'] = self.unpack.int16(packet, byteCount)/100 #6 bytes
                byteCount+=2
                #print(packet[byteCount:byteCount+2])
                payload['temprature']=self.unpack.int16(packet, byteCount)/100 #2 bytes (Values were multiplied by 100 to keep the decimal)
                byteCount+=2
                #print(packet[byteCount:byteCount+1])
                payload['humidity']=self.unpack.uint8(packet, byteCount) #1 byte
                byteCount+=1
           
                #payload['preasure']=self.unpack.uint16(packet, byteCount) #2 bytes
                #byteCount+=2
                #payload['humidity']=self.unpack.uint8(packet, byteCount) #1 byte
                #byteCount+=1
                #payload['vocConcentration']=self.unpack.uint16(packet, byteCount) #2 bytes
                #byteCount+=2
                #payload['co2Concentration']= self.unpack.uint16(packet, byteCount) #2 bytes
                #byteCount+=2

                self.add_data(payload)
                print(payload)
                return 'data'


            case 0x01:

                payload['packetCount'] = self.unpack.uint16(packet, byteCount) #2 bytes
                byteCount+=2

                payload['baterryVoltage'] = self.unpack.float32(packet, byteCount) #2 bytes (Values were multiplied by 100 to keep the decimal)
                byteCount+=4

                payload['memUsage'] = self.unpack.uint16(packet, byteCount) #2 bytes
                byteCount+=2

                payload['gy'] = bool(self.unpack.bit(packet, byteCount, 0))
                payload['dht'] = bool(self.unpack.bit(packet, byteCount, 1))
                payload['gps'] = bool(self.unpack.bit(packet, byteCount, 2))
                payload['sd'] = bool(self.unpack.bit(packet, byteCount, 3))
                byteCount+=1
                #All above combined are 1 byte 
                print(payload)
                #payload['message'] = self.unpack.string(packet, byteCount, header['length'])
                byteCount+=(header['length']-byteCount)

                self.debug_data(payload)
                return 'debug'
            
        
            

    #A function that adds data to the main DataStore
    def add_data(self, data: dict):
        #Add the data to 
        self.DataBase.add(data)
        self.dictData.append(data)
        with open(self.path + 'data.json', 'w') as f:
            json.dump(self.dictData, f, indent=4)
        
    
    #A function that deals with debug data
    def debug_data(self, data: dict):
        #Add the data to 
        self.DebugData.add(data)
        self.dictDebug.append(data)
        with open(self.path + 'debug.json', 'w') as f:
            json.dump(self.dictDebug, f, indent=4)
            
    def extraxtData(self, keyword:str):
        getter = itemgetter(keyword)
        
        return np.array(list(map(getter, self.DataBase)))
            
def SerialSetup():
    #Find available ports and promt user to choose
    ports = list_ports.comports()
    print("\nChoose port:")
    for i, port in enumerate(ports):
        if port.description!="n/a":
            print(f"({port.description})")
            print(f"-{i}- {port.name} {port.description}")
        
    ser = Serial(f"/dev/{ports[int(input('Choose:'))].name}", 57600)
    
    #Wait till serial initialises
    i = 0
    while (not ser.is_open):
        logging.debug("Waiting for serial to begin:", i)
        i+=1
        sleep(1)
    
    logging.debug("Serial initialised!")

    
    return ser

if __name__ == "__main__":
    
            

        pass