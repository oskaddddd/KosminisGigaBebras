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

import logging

import time

logging.basicConfig(level=logging.DEBUG)



startBytes = [0x98, 0x98]
canID = 0xff

fullID = bytes([byte for byte in startBytes]+[canID])
DataManager = dataAPI.DataMain(startBytes, canID)



class dataPlotWidget(pg.PlotWidget):
    def __init__(self, parent=None, background='default', plotItem=None, **kargs):
        super().__init__(parent, background, plotItem, **kargs)
        
    
        
    




class gpsWidget(gl.GLViewWidget):
    def __init__(self, *args, devicePixelRatio=None, **kwargs):
        super().__init__(*args, devicePixelRatio=devicePixelRatio, **kwargs)
        grid = gl.GLGridItem()
        grid.setSize(800, 800)
        grid.setSpacing(10, 10)
        self.addItem(grid)
        
        self.plot = gl.GLLinePlotItem(antialias = False, color = (255, 0, 0, 255), mode = 'line_strip')
        

        self.addItem(self.plot)
    def paintGL(self):
        self.makeCurrent()
        super().paintGL()
        
            
             
        
            
    
    def setTimeDot(time):
        pass
    
        
class SerialMonitor(QThread):
    update_signal = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        
        
        self.running = True
        self.pollingRate  = 30 #hertz
    
    
    def run(self):
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
                pType = DataManager.parse_packet(packet)
                #print(round(data["acceleration"][0], 2), round(data["acceleration"][1], 2), round(data["acceleration"][2], 2))
                #print(data)
                if pType:
                    self.update_signal.emit(pType)
                    
                    
                    
                
                
            time.sleep(1/self.pollingRate)
    
    
    #        
    #def start(self):
    #    self.running = True
    #    self.run()
        
    
class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.ui = uic.loadUi('GroundMain/assets/UI.ui', self)
        
       
        self.serial = SerialMonitor()
        self.serial.update_signal.connect(self.updateData)
        self.ui.dataDropdown.currentTextChanged.connect(self.dataDropboxChenged)
 

        self.timeline = np.array([])
        self.debugPlotData = []
        
        
        self.dataType = "height"
        
        
        if len(DataManager.dictData) != 0:
            self.updateData("data")
        
       
        self.serial.start()
        
    def updateLines(self):
        
        #Extract gps and height data into a np array
        gps = DataManager.extraxtData("gps")
        height = DataManager.extraxtData("height")
        
        #print(gps)
        #print(height)
        #print("------------")

        result = np.column_stack((gps, height))
        

        #Normalise the data
        if len(gps) != 0 and len(height) != 0:
            for i in range(3):
                result[:, i] -= result[len(result)-1][i]
                print(result)
            result[:, :2] //= 100
            result[:, 2] //= 10

            self.ui.locationPlot.plot.setData(pos = result)
        print(result)
            
    def updateData(self, pType):
        
        tStamp = round(time.time())
        
        #if len(self.debugPlotData) < tStamp+1:
        #    self.debugPlotData.append(0)
        #self.debugPlotData[tStamp] += 1
        #
        #self.ui.debugPlot.clear()
        #self.ui.debugPlot.plot(list(range(tStamp)), self.debugPlotData)
        
        if pType == 'data':
            self.updateLines()

            self.timeline = DataManager.extraxtData("timestamp")[:]/1000
            
            self.ui.dataPlot.clear()
            self.ui.dataPlot.plot(self.timeline, DataManager.extraxtData(self.dataType))
            
        
        elif pType == 'debug':
            self.ui.ramLabel.setText(f"RAM: {round((1-(DataManager.DebugData[0]['memUsage']/2048))*100)}%")
            #self.ui.vccLabel.setText(f"VCC: {round(DataManager.DebugData[0]['baterryVoltage']}")
            self.ui.lossLabel.setText(f"LOSS: {round(1-(DataManager.packetCount/DataManager.DebugData[0]['packetCount']), 4)*100}%")
            self.ui.gyCheckbox.setChecked(DataManager.DebugData[0]["gy"])
            self.ui.dhtCheckbox.setChecked( DataManager.DebugData[0]["dht"])
            self.ui.gpsCheckbox.setChecked( DataManager.DebugData[0]["gps"])
            self.ui.sdCheckbox.setChecked(DataManager.DebugData[0]["sd"])
            
            print(f"PACKETS:: {DataManager.packetCount} {DataManager.DebugData[0]['packetCount']}")
        
        #self.ui.debugPlot.plot(self.time)
        
        
        
        
    def dataDropboxChenged(self, text):
        self.dataType = text
        self.ui.dataPlot.clear()
        self.ui.dataPlot.plot(self.timeline, DataManager.extraxtData(self.dataType))

if __name__ == "__main__":
  

    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    qdarktheme.setup_theme()
    
    window = MainWindow()
    window.show()
    app.exec()
    