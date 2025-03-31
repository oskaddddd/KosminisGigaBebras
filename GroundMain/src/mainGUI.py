from PyQt6.QtWidgets import *
from PyQt6 import uic
from PyQt6.QtCore import QThread, pyqtSignal

import sys
import qdarktheme

import dataAPI

from time import sleep

import logging

logging.basicConfig(level=logging.DEBUG)



startBytes = [0x98, 0x98]
canID = 0xff

fullID = bytes([byte for byte in startBytes]+[canID])
DataManager = dataAPI.DataMain(startBytes, canID)


class SerialMonitor(QThread):
    update_signal = pyqtSignal(dict)
    
    def __init__(self):
        super().__init__()
        
        self.coms = dataAPI.SerialSetup()
        self.running = True
        self.pollingRate  = 30 #hertz
        self.run()
        
    
    def run(self):
        while self.running:
            if self.coms.in_waiting > 0:
                packet = self.coms.read_until(fullID)
                print()
                #check if /r/n is before start bytes
                if packet[(len(packet)-len(fullID)-2):(len(packet)-len(fullID))] != b'\r\n':
                    logging.warning("\\r\\n not before start bytes", packet[(len(packet)-len(fullID)-2):(len(packet)-len(fullID))])
                    continue
                packet = fullID + packet[:(len(packet)-len(fullID))]
                for i, byte  in enumerate(packet):
                    
                    print(i, f"\\x{hex(byte)}")
                data = DataManager.parse_packet(packet)
                print(data)
                if data:
                    self.update_signal.emit(data)
                
                
            sleep(1/self.pollingRate)
    #        
    #def start(self):
    #    self.running = True
    #    self.run()
        
    
class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        uic.loadUi('GroundMain/assets/UI.ui', self)
        
        self.serial = SerialMonitor()
        self.serial.update_signal.connect(self.updateData)
        
        self.serial.start()

    def updateData(self, data:dict):
        print("whahsd")
        print("Hello", data)

if __name__ == "__main__":
   # coms = dataAPI.SerialSetup()
   # running = True
   # pollingRate  = 30 #hertz
   # while True:
   #         if coms.in_waiting > 0:
   #             packet = coms.readline()
   #             print(packet, "\n\n")
   #             #data = DataManager.parse_data(packet)
   #             
#
   #             
   #             
   #         sleep(1/pollingRate)
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    qdarktheme.setup_theme()
    
    window = MainWindow()
    app.exec()
    