from PyQt6.QtWidgets import *
from PyQt6 import uic
from PyQt6.QtCore import QThread, pyqtSignal

import sys
import qdarktheme

import dataAPI

from time import sleep

import logging



DataManager = dataAPI.DataMain()


class SerialMonitor(QThread):
    update_signal = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
 
        self.coms = dataAPI.SerialSetup()
        self.running = True
        self.pollingRate  = 30 #hertz
        
    
    def run(self):
        while self.running:
            if self.coms.in_waiting > 0:
                packet = self.coms.readline()
                print(packet)
                data = DataManager.parse_data(packet)
                
                self.update_signal.emit(data)
                
                
            sleep(1/self.pollingRate)
            
    def start(self):
        self.running = True
        self.run()
        
    
class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        uic.loadUi('GroundMain/assets/UI.ui', self)
        
        self.serial = SerialMonitor()
        self.serial.update_signal.connect(self.updateData)
        
        self.serial.start()

<<<<<<< Updated upstream
    def updateData(self, data:dict):
        print(data)
=======
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
                if self.dataType in ["acceleration", "magneticField"]:
                    #dat = DataManager.extraxtData(self.dataType, np.float32)
                    #magnitudes = np.linalg.norm(dat, axis=1)
                    #print("dat", dat, "\nmag:", magnitudes)
                    self.ui.dataPlot.curve.setData(\
                        x = self.timeline,\
                        y = np.linalg.norm(DataManager.extraxtData(self.dataType, np.float32), axis=1))
                else:
                    self.ui.dataPlot.curve.setData(\
                        x = self.timeline, \
                        y = DataManager.extraxtData(self.dataType))
                #self.ui.dataPlot.curve.setData(x = self.timeline, y = DataManager.extraxtData(self.dataType))
                
            
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
                self.ui.co2Checkbox.setChecked(DataManager.DebugData[0]["co2"])
                
            
        #Plot the {packets per second} graphs
        #self.ui.debugPlot.curve1.setData(x = list(range(tStamp+1)), y = self.debugPlotDebug, pen = self.pens[0])
        #self.ui.debugPlot.plot(list(range(tStamp+1)), self.debugPlotDebug, pen = self.pens[1])
        #self.ui.debugPlot.marker(self.ui.timeSlider.time/1000)
        
        
    #Fucntion handling the change of the data type selection dropdown
    def dataDropboxChenged(self, text):
        self.dataType = text
        if self.dataType in ["acceleration", "magneticField"]:
            #dat = DataManager.extraxtData(self.dataType, np.float32)
            #magnitudes = np.linalg.norm(dat, axis=1)
            #print("dat", dat, "\nmag:", magnitudes)
            self.ui.dataPlot.curve.setData(\
                x = self.timeline,\
                y = np.linalg.norm(DataManager.extraxtData(self.dataType, np.float32), axis=1))
        else:
            self.ui.dataPlot.curve.setData(\
                x = self.timeline, \
                y = DataManager.extraxtData(self.dataType))
            
        self.ui.dataPlot.centerOn(self.ui.dataPlot.curve)
        self.ui.dataPlot.plotItem.enableAutoRange('xy', True)
        self.ui.dataPlot.plotItem.autoRange()
        
>>>>>>> Stashed changes

if __name__ == "__main__":
    coms = dataAPI.SerialSetup()
    running = True
    pollingRate  = 30 #hertz
    while True:
            if coms.in_waiting > 0:
                packet = coms.readline()
                print(packet, "\n\n")
                #data = DataManager.parse_data(packet)
                

                
                
            sleep(1/pollingRate)
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    qdarktheme.setup_theme()
    
    window = MainWindow()
    app.exec()
    