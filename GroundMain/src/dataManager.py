import struct
from sortedcontainers import SortedList


class DataMain():
    def __init__(self):
        
        #Object where all of the collected data is stored
        self.DataStore = SortedList()
    
    #A function that proccesses incoming packets from the Can 
    def decode_data(self, packet:bytes):
        pass
    
    #A function that adds data to the main DataStore
    def add_data(self, data: object):
        pass
    
    #A function that deals with debug data
    def debug_data(self, data):
        pass
        
        