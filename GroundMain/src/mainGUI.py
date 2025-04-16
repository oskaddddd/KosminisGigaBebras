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


#2D plot widget
class dataPlotWidget(pg.PlotWidget):
    def __init__(self, parent=None, background='default', plotItem=None, **kargs):
        super().__init__(parent, background, plotItem, **kargs)
        self.plotItem.showGrid(x=True, y=True, alpha=0.5)
        
    
#Time slider widget      
class timeSlider(QSlider):
    timeUpdated = pyqtSignal(int)
    def __init__(self, parent=None):
        super().__init__(parent),
        
        self.time = DataManager.DataBase[0]["timestamp"]
        self.timeRange = DataManager.DataBase[0]["timestamp"] - DataManager.DataBase[len(DataManager.DataBase)-1]["timestamp"]
        
        self.maxValue = 1000
        
        self.setValue(self.maxValue)
        
        self.valueChanged.connect(self.updateSliderVal)
        

        
    #Called to update the slide object, when new data is recieved and timestamp range expands
    def updateTimerange(self):
        self.timeRange = DataManager.DataBase[0]["timestamp"] - DataManager.DataBase[len(DataManager.DataBase)-1]["timestamp"]
        self.setValue(round(self.time/self.timeRange*self.maxValue))
        #DataManager.DataBase.bisect_left({"timestamp":100})
    
    def inputTimestamp(self, timestamp):
        index = DataManager.DataBase.bisect_left({"timestamp":timestamp})
        
    #Called when the slider value is changed
    def updateSliderVal(self, value):
        expectedTime = (self.timeRange*value/self.maxValue) + DataManager.DataBase[len(DataManager.DataBase)-1]["timestamp"]
        index = DataManager.DataBase.bisect_left({"timestamp":expectedTime })
        
        self.time = DataManager.DataBase[index]["timestamp"]
        logging.debug(f"ratio:{value/self.maxValue}\ntimerange:{self.timeRange}\nexpectedTime:{expectedTime}\TIME:{self.time}\nIndex:{index}\nRangeLen:{len(DataManager.DataBase)}")
        self.timeUpdated.emit(self.time)
        #print(self.time)
        #print()
        
    
        
        
        
#3D plot widget for GPS
class gpsWidget(gl.GLViewWidget):
    def __init__(self, *args, devicePixelRatio=None, **kwargs):
        super().__init__(*args, devicePixelRatio=devicePixelRatio, **kwargs)
        grid = gl.GLGridItem()
        grid.setSize(800, 800)
        grid.setSpacing(1, 1)
        self.addItem(grid)
        
        self.plot = gl.GLLinePlotItem(antialias = False, color = (255, 0, 0, 255), mode = 'line_strip')
        

        self.addItem(self.plot)
    def paintGL(self):
        self.makeCurrent()
        super().paintGL()
    
    def setTimeDot(time):
        pass
    
     
#Class responsible for parsing packets   
class SerialMonitor(QThread):
    update_signal = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()

        self.running = True
        self.pollingRate = 30 #hertz
    
    
    def run(self):
        #Setup serial
        self.coms = dataAPI.SerialSetup()
        
        #Serial loop
        while self.running:
            #If data iavailable
            if self.coms.in_waiting > 0:
                
                #Read until the startbytes of a new packet are reached
                packet = self.coms.read_until(fullID)
                print()
                #Check if /r/n (endbytes) are before the start bytes
                if packet[(len(packet)-len(fullID)-2):(len(packet)-len(fullID))] != b'\r\n':
                    logging.warning("\\r\\n not before start bytes", packet[(len(packet)-len(fullID)-2):(len(packet)-len(fullID))])
                    continue
                
                #Remove the startbytes from the end of the packet and since they were lost from the beggining of the packet, add them back
                #(startbytes + packet - startbytes)
                packet = fullID + packet[:(len(packet)-len(fullID))]
                
                #Debug the packet to the terminal
                for i, byte  in enumerate(packet):
                    logging.debug(i, f"\\x{hex(byte)}")
                
                #Parse the packet
                #Get the type of packet ('data', 'debug')
                pType = DataManager.parse_packet(packet)
                
                #If PacketType is valid, send it back
                if pType:
                    self.update_signal.emit(pType)
                    
                    
                    
                
                
            time.sleep(1/self.pollingRate)
    
    
        
    
class MainWindow(QMainWindow):
    def __init__(self):
        #Init the UI
        super(MainWindow, self).__init__()
        self.ui = uic.loadUi('GroundMain/assets/UI.ui', self)
        
        #Get the start time of the code to display packets/second graph 
        self.startTime = round(time.time())
        #Arrays for staring data for amount of packets recieved per second
        self.debugPlotData = [0]
        self.debugPlotDebug = [0]
        self.pens = [pg.mkPen(color = "w"), pg.mkPen(color = 'r')]

        #Initialise the serialMonitor Class responsible for managing incoming data
        self.serial = SerialMonitor()
        self.serial.update_signal.connect(self.updateData)
        
        #Connect the data selection dropdown to a function responsible for changing the data on the graph
        self.ui.dataDropdown.currentTextChanged.connect(self.dataDropboxChenged)
        self.dataType = "height"
        
    
 
        #A list of all the timestamps recieved from the CanSat for the X axis of graphs 
        #(not related to starttime. Startime is local time, while timeline is as reported by cansat)
        self.timeline = np.array([])
        
        #If data was not cleared, display it
        if len(DataManager.dictData) != 0:
            self.updateData("data")
        
        self.ui.timeSlider.timeUpdated.connect(self.setDotsOnGraphs)
        
        #Start listening for packets
        self.serial.start()
        
        
    def setDotsOnGraphs(self, timestamp):
        print(f"time: {timestamp}")
        
    def updateGpsPlot(self):
        
        #Extract gps and height data into a np array
        gps = DataManager.extraxtData("gps")
        height = DataManager.extraxtData("height")
        result = np.column_stack((gps, height))
        

        #Normalise the data
        if len(gps) != 0 and len(height) != 0:
            for i in range(3):
                #Normalise so that the canSat is always at (0;0)
                result[:, i] -= result[0][i]
            
            #Gps returned with 6 digits after decimal accuracy (as an int *10^6)
            #6 dec - 0.11m; 5 dec - 1.1m; 4 dec - 11m...
            #So gps devided by 100, 1 unit = 11m
            result[:, :2] //= 100
            
            #Height devided by 10, so 1 unit = 10 meters
            result[:, 2] //= 10
            
            #DECIAML IS NOT KEPT, EVERYTHING IS IN INT FORM

            #Plot the data
            self.ui.locationPlot.plot.setData(pos = result)
       
    #Handles ploting when new data is recieved      
    def updateData(self, pType):
        
        #Calculate the local time (Since start of program)
        tStamp = round(time.time()) - self.startTime
        
        #Expand {packets per second} arrays, till their lentgh is equal to program runtime in seconds
        while len(self.debugPlotData) < tStamp+1:
            self.debugPlotData.append(0)
        while len(self.debugPlotDebug) < tStamp+1:
            self.debugPlotDebug.append(0)
        
        
        match pType:
            #Eecieved a data packet
            case "data":
                #Add 1, to count of data packets recieved this second
                self.debugPlotData[tStamp] += 1
                
                #Update the gps plot
                self.updateGpsPlot()
                
                #Update the entire timeline (x axis of graphs displaying CanSat data)
                self.timeline = DataManager.extraxtData("timestamp")[:]/1000
                
                #Clear the data plot and draw a new graph with the updated data
                self.ui.dataPlot.clear()
                self.ui.dataPlot.plot(self.timeline, DataManager.extraxtData(self.dataType))
            
            #Recieved a debug packet 
            case "debug":
                #Add 1, to count of debug packets recieved this second
                self.debugPlotDebug[tStamp] += 1
            
                #Set all the sensor checkboxes to their status
                self.ui.ramLabel.setText(f"RAM: {round((1-(DataManager.DebugData[0]['memUsage']/2048))*100)}%")
                #self.ui.vccLabel.setText(f"VCC: {round(DataManager.DebugData[0]['baterryVoltage']}")
                self.ui.lossLabel.setText(f"LOSS: {round(1-(DataManager.packetCount/DataManager.DebugData[0]['packetCount']), 4)*100}%")
                self.ui.gyCheckbox.setChecked(DataManager.DebugData[0]["gy"])
                self.ui.dhtCheckbox.setChecked( DataManager.DebugData[0]["dht"])
                self.ui.gpsCheckbox.setChecked( DataManager.DebugData[0]["gps"])
                self.ui.sdCheckbox.setChecked(DataManager.DebugData[0]["sd"])
                
            
        #Plot the {packets per second} graphs
        self.ui.debugPlot.clear()
        self.ui.debugPlot.plot(list(range(tStamp)), self.debugPlotData, pen = self.pens[0])
        self.ui.debugPlot.plot(list(range(tStamp)), self.debugPlotDebug, pen = self.pens[1])
        
        
    #Fucntion handling the change of the data type selection dropdown
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
    