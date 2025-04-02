from PyQt6.QtWidgets import *
from PyQt6 import uic
from PyQt6.QtCore import QThread, pyqtSignal

from pyqtgraph import PlotWidget
import pyqtgraph as pg
import pyqtgraph.opengl as gl

import sys
import numpy as np

import qdarktheme

import dataAPI

from time import sleep

import logging
from sortedcontainers import sortedlist
from operator import itemgetter

logging.basicConfig(level=logging.DEBUG)



startBytes = [0x98, 0x98]
canID = 0xff

fullID = bytes([byte for byte in startBytes]+[canID])
DataManager = dataAPI.DataMain(startBytes, canID)



class gpsWidget(gl.GLViewWidget):
    def __init__(self, *args, devicePixelRatio=None, **kwargs):
        super().__init__(*args, devicePixelRatio=devicePixelRatio, **kwargs)
        grid = gl.GLGridItem()
        grid.setSize(800, 800)
        grid.setSpacing(10, 10)
        self.addItem(grid)
        
        self.plot = gl.GLLinePlotItem(pos = [[0, 0, 0], [2, 4, 5], [3, 3, 0], [0, 0, 0]], antialias = False, color = (255, 0, 0, 255), mode = 'line_strip')
        

        self.addItem(self.plot)
    def paintGL(self):
        self.makeCurrent()
        super().paintGL()
        
    def updateLines():
        gpsGetter = itemgetter("gps")
        heightGetter = itemgetter("height")
        
        #Extract gps and height data into a np array
        gps = np.array(list(map(list, map(gpsGetter, DataManager.DataBase))))
        height = np.array(list(map(list, map(heightGetter, DataManager.DataBase))))

        result = np.column_stack((gps, height))

        #Normalise the data
        for i in range(3):
            result[:, i] -= result[0][i]
            
        print(result)
             
        
            
    
    def setTimeDot(time):
        pass
    
        
class SerialMonitor(QThread):
    update_signal = pyqtSignal(dict)
    
    def __init__(self):
        super().__init__()
        
        
        self.running = True
        self.pollingRate  = 30 #hertz
    
    
    def start(self):
        self.coms = dataAPI.SerialSetup()
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
                #print(round(data["acceleration"][0], 2), round(data["acceleration"][1], 2), round(data["acceleration"][2], 2))
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
    
        #self.serial.start()

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
    window.show()
    app.exec()
    