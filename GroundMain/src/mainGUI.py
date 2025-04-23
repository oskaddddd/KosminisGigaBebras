from PyQt6.QtWidgets import *
from PyQt6 import uic
from PyQt6.QtCore import QThread, pyqtSignal
from PyQt6.QtGui import QVector3D

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
print("Hello")

#2D plot widget
class dataPlotWidget(PlotWidget):
    
    
    def __init__(self, parent=None, background='default', plotItem=None, **kargs):
        super().__init__(parent, background, plotItem, **kargs)
        self.plotItem.showGrid(x=True, y=True, alpha=0.5)
        
        self.curve = pg.PlotDataItem(pen = pg.mkPen('g', ))
        self.addItem(self.curve)
        self.curve1 = pg.PlotDataItem(pen = pg.mkPen('b', ))
        self.addItem(self.curve1)
        self.markerLine = pg.InfiniteLine(pos=0, pen = pg.mkPen('r'))
        self.addItem(self.markerLine)
        
        
        #self.marker.__init__()
    def marker(self, x):
        self.markerLine.setPos(x)
        #self.plotItem.addLine(x = x, y=None,  pen=pg.mkPen('r', width=1))
    
#Time slider widget      
class timeSlider(QSlider):
    timeUpdated = pyqtSignal(int)
    def __init__(self, parent=None):
        super().__init__(parent),
        
class SliderManager():
    def __init__(self, slider:timeSlider):
        self.time = 0
        self.timeRange = 0
        if len(DataManager.DataBase) != 0:
            self.time = DataManager.DataBase[0]["timestamp"]
            self.timeRange = DataManager.DataBase[0]["timestamp"] - DataManager.DataBase[-1]["timestamp"]
        
        
        self.slider = slider
        self.slider.valueChanged.connect(self.updateSliderVal)
        self.slider.setMaximum(len(DataManager.DataBase))
    #Called to update the slide object, when new data is recieved and timestamp range expands
    def updateTimerange(self):
        if not len(DataManager.DataBase):
            logging.debug("Not updating time range, len(DataBase) = 0")
            return
        
        #Function to handle if slider is at a maximum
        def doMaximum():
            self.time = DataManager.DataBase[0]["timestamp"]
            self.slider.blockSignals(True)
            self.slider.setValue(self.slider.maximum())
            self.slider.blockSignals(False)
            self.slider.timeUpdated.emit(0)
        
        logging.debug("Updating time range")
        #Calculate the timerange
        self.timeRange = DataManager.DataBase[0]["timestamp"] - DataManager.DataBase[len(DataManager.DataBase)-1]["timestamp"]
        
        maximum = False
        #Check if slider is at the maximum and store it
        if self.slider.value() == self.slider.maximum():
            maximum = True
        
        #Extend the slider range to match the amount of data, with a max of 1000
        if len(DataManager.DataBase) < 1000:
            self.slider.setMaximum(len(DataManager.DataBase))
            
            if maximum:
                doMaximum()
                return
            
        else:
            if maximum:
                doMaximum()
                return
            
            self.slider.blockSignals(True)
            #Handle division by 0 case
            if self.timeRange == 0:
                self.slider.setValue(self.slider.maximum())
            #Calculate and set the value of the slider
            else:
                self.slider.setValue(round((self.time-DataManager.DataBase[-1]["timestamp"])/self.timeRange*self.slider.maximum()))
            self.slider.blockSignals(False)
        #DataManager.DataBase.bisect_left({"timestamp":100})
    
    def inputTimestamp(self, timestamp):
        index = DataManager.DataBase.bisect_left({"timestamp":timestamp})
        
    #Called when the slider value is changed
    def updateSliderVal(self, value):
        if self.timeRange == 0:
            return
        
        expectedTime = (self.timeRange*value/self.slider.maximum()) + DataManager.DataBase[-1]["timestamp"]
        index = DataManager.DataBase.bisect_left({"timestamp":expectedTime })
        
        self.time = DataManager.DataBase[index]["timestamp"]
        logging.debug(f"ratio:{value/self.slider.maximum()}\n-timerange:{self.timeRange}\n-expectedTime:{expectedTime}\n-TIME:{self.time}\n-Index:{index}\n-RangeLen:{len(DataManager.DataBase)}")
        self.slider.timeUpdated.emit(index)
        
    
        
        
        
#3D plot widget for GPS
class gpsWidget(gl.GLViewWidget):
    def __init__(self, *args, devicePixelRatio=None, **kwargs):
        super().__init__(*args, devicePixelRatio=devicePixelRatio, **kwargs)
        grid = gl.GLGridItem()
        grid.setSize(800, 800)
        grid.setSpacing(1, 1)
        self.addItem(grid)
        
        self.plot = gl.GLLinePlotItem(antialias = False, color = (255, 255, 255, 255), mode = 'line_strip')
        self.markerDot = gl.GLScatterPlotItem(pos = np.array([[0, 0, 0]]), size = 10, color = (255, 0, 0, 255))
        #self.markerDot.setData(pos = [[0, 0, 0]], size = 1, color = 'r')

        self.addItem(self.markerDot)
        

        self.addItem(self.plot)
    def paintGL(self):
        self.makeCurrent()
        super().paintGL()
        

    
     
#Class responsible for parsing packets   
class SerialMonitor(QThread):
    update_signal = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()

        self.running = True
        self.pollingRate = 30 #hertz
    
    
    def run(self):
        #Setup serial
        print("asd")
        self.coms = dataAPI.SerialSetup("ttyUSB0")
        print("miau")
        
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
                #for i, byte  in enumerate(packet):
                #    logging.debug(i, f"\\x{hex(byte)}")
                
                #Parse the packet
                #Get the type of packet ('data', 'debug')
                pType = DataManager.parse_packet(packet)
                
                #If PacketType is valid, send it back
                if pType:
                    self.update_signal.emit(pType)
                    
                    
                    
                
            
            time.sleep(1/self.pollingRate)
    
class one_second_loop(QThread):
    signal = pyqtSignal()
    def __init__(self):
        super().__init__()
    def run(self):
        while 1:
            self.signal.emit()
            time.sleep(1)
            
    
class MainWindow(QMainWindow):
    def __init__(self):
        #Init the UI
        super(MainWindow, self).__init__()
        self.ui = uic.loadUi('GroundMain/assets/UI.ui', self)
        
        #Get the start time of the code to display packets/second graph 
        self.startTime = round(time.time())
        #Arrays for staring data for amount of packets recieved per second
        self.debugPlotData = [0]
        #self.debugPlotDebug = [0]
        self.pens = [pg.mkPen(color = "w"), pg.mkPen(color = 'r'), pg.mkPen(color = 'b')]
        
        

        #Initialise the serialMonitor Class responsible for managing incoming data
        self.serial = SerialMonitor()
        self.serial.update_signal.connect(self.updateData)
        
        #Connect the data selection dropdown to a function responsible for changing the data on the graph
        self.ui.dataDropdown.currentTextChanged.connect(self.dataDropboxChenged)
        self.dataType = "height"
        
        #self.ui.debugPlot.curve.pen = self.pens[1]
        #self.ui.debugPlot.curve1.pen = self.pens[2]
        
        self.valid_gps_index = 0
        
        self.sliderManager = SliderManager(self.ui.timeSlider)
        
        
    
 
        #A list of all the timestamps recieved from the CanSat for the X axis of graphs 
        #(not related to starttime. Startime is local time, while timeline is as reported by cansat)
        self.timeline = np.array([])
        
        #If data was not cleared, display it
        if len(DataManager.dictData) != 0:
            self.updateData("data")
        
        self.ui.timeSlider.timeUpdated.connect(self.updateGraphMarkers)
        
        #Start listening for packets
        self.serial.start()
        
        self.one_second_loop = one_second_loop()
        self.one_second_loop.signal.connect(self.updateDebugPlot)
        self.one_second_loop.start()
        
        
    def updateGraphMarkers(self, index):
        dot = DataManager.DataBase[index]
        gpsDot = np.array([[*dot["gps"], dot["height"]]], dtype=np.float32)
        gpsDot[0]-=np.array([*DataManager.DataBase[0]["gps"], DataManager.DataBase[0]["height"]])
        gpsDot[0, :2] /= 100
        gpsDot[0, 2] /= 10
        print(gpsDot)
        self.ui.locationPlot.markerDot.setData(pos = gpsDot)
        
        self.ui.locationPlot.setCameraPosition(pos = QVector3D(*gpsDot[0]))
        self.ui.dataPlot.marker(self.sliderManager.time/1000)
        #self.ui.debugPlot.marker(self.sliderManager.time/1000)
        
        
        
    def updateGpsPlot(self):
        
        #Extract gps and height data into a np array
        
        gps = DataManager.extraxtData("gps", np.float32)
        
        #Ignore the invalid gps data
        if len(gps) != 0 and gps[0][0] == 0:
            self.valid_gps_index = len(gps)-1
            return
        
        height = DataManager.extraxtData("height", np.float32)
        
        startIndex = len(gps)-self.valid_gps_index
        result = np.column_stack((gps[:startIndex], height[:startIndex]))
        
        
        #Normalise the data
        if len(gps) != 0 and len(height) != 0:
            for i in range(3):
                #Normalise so that the canSat is always at (0;0)
                result[:, i] -= result[0][i]
            
            #Gps returned with 6 digits after decimal (as an int *10^6)
            #6 dec - 0.11m; 5 dec - 1.1m; 4 dec - 11m...
            #So gps devided by 100, 1 unit = 11m
            result[:, :2] /= 100
            
            #Height devided by 10, so 1 unit = 10 meters
            result[:, 2] /= 10

            #Plot the data
            self.ui.locationPlot.plot.setData(pos = result)
    def updateDebugPlot(self, newPackets = 0):
                
        #Calculate the local time (Since start of program)
        tStamp = round(time.time()) - self.startTime
        
        #Expand {packets per second} arrays, till their lentgh is equal to program runtime in seconds
        while len(self.debugPlotData) < tStamp+1:
            self.debugPlotData.append(0)
        #while len(self.debugPlotDebug) < tStamp+1:
        #    self.debugPlotDebug.append(0)
        self.debugPlotData[tStamp] += newPackets
        
        self.ui.debugPlot.curve.setData(x = list(range(tStamp+1)), y = self.debugPlotData)
        
    #Handles ploting when new data is recieved      
    def updateData(self, pType):
 
        self.sliderManager.updateTimerange()
        self.updateDebugPlot(1)
                
        match pType:
            
            #Eecieved a data packet
            case "data":
                
                #Update the gps plot
                self.updateGpsPlot()
                
                #Update the entire timeline (x axis of graphs displaying CanSat data)
                self.timeline = DataManager.extraxtData("timestamp")[:]/1000
                
                #Clear the data plot and draw a new graph with the updated data
                
                self.ui.dataPlot.curve.setData(x = self.timeline, y = DataManager.extraxtData(self.dataType))
                
            
            #Recieved a debug packet 
            case "debug":
                ##Add 1, to count of debug packets recieved this second
                #self.debugPlotDebug[tStamp] += 1
            
                #Set all the sensor checkboxes to their status
                self.ui.ramLabel.setText(f"RAM: {round((1-(DataManager.DebugData[0]['memUsage']/2048))*100)}%")
                self.ui.vccLabel.setText(f"VCC: {DataManager.DebugData[0]['baterryVoltage']}")
                loss = (1 - (DataManager.packetCount / DataManager.DebugData[0]['packetCount'])) * 100
                self.ui.lossLabel.setText(f"LOSS: {loss:.2f}%")
                self.ui.gyCheckbox.setChecked(DataManager.DebugData[0]["gy"])
                self.ui.dhtCheckbox.setChecked( DataManager.DebugData[0]["dht"])
                self.ui.gpsCheckbox.setChecked( DataManager.DebugData[0]["gps"])
                self.ui.sdCheckbox.setChecked(DataManager.DebugData[0]["sd"])
                
            
        #Plot the {packets per second} graphs
        #self.ui.debugPlot.curve1.setData(x = list(range(tStamp+1)), y = self.debugPlotDebug, pen = self.pens[0])
        #self.ui.debugPlot.plot(list(range(tStamp+1)), self.debugPlotDebug, pen = self.pens[1])
        #self.ui.debugPlot.marker(self.ui.timeSlider.time/1000)
        
        
    #Fucntion handling the change of the data type selection dropdown
    def dataDropboxChenged(self, text):
        self.dataType = text
        self.ui.dataPlot.curve.setData(x = self.timeline, y = DataManager.extraxtData(self.dataType))
        self.ui.dataPlot.centerOn(self.ui.dataPlot.curve)
        self.ui.dataPlot.plotItem.enableAutoRange('xy', True)
        self.ui.dataPlot.plotItem.autoRange()
        

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    qdarktheme.setup_theme()
    
    window = MainWindow()
    window.show()
    app.exec()
    