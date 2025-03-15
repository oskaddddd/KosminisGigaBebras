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
        self.running = False
        self.pollingRate  = 30 #hertz
    
    def run(self):
        while self.running:
            if self.coms.in_waiting > 0:
                packet = self.coms.readline()
                data = DataManager.parse_packet(packet)
                
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

    def updateData(self, data:dict):
        pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    qdarktheme.setup_theme()
    
    window = MainWindow()
    app.exec()
    