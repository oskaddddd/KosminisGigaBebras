from PyQt6.QtWidgets import *
from PyQt6 import uic
from PyQt6.QtCore import QThread, pyqtSignal

import sys
import qdarktheme

import dataAPI



DataManager = dataAPI.DataMain()


class SerialMonitor(QThread):
    update_signal = pyqtSignal(str)
    
    def __init__(self):
        self.coms = dataAPI.SerialSetup()
        self.running = True
    
    def run(self):
        while self.running:
            #Do serial monoty things
            pass
        
    
    def miau(self):
        #Do work
        self.update_signal.emit("message")
    
    
    


class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        uic.loadUi('UI/UI.ui', self)
        
        self.serial = SerialMonitor()
        self.serial.update_signal.connect(self.updateData)

    def updateData(self, message):
        pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    qdarktheme.setup_theme()
    
    window = MainWindow()
    app.exec()
    