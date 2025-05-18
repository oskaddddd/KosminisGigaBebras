from PyQt6.QtWidgets import QApplication, QWidget, QLabel
from PyQt6.QtGui import QPainter, QLinearGradient, QColor, QFont
from PyQt6.QtCore import Qt
import sys

import pyqtgraph as pg
print(pg.colormap.listMaps())
class GradientColorbar(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(80, 220)

    def paintEvent(self, event):
        painter = QPainter(self)

        # Draw the gradient rectangle
        gradient = QLinearGradient(20, 10, 20, 200)  # vertical gradient
        gradient.setColorAt(0.0, QColor(255, 0, 0))     # Red
        gradient.setColorAt(0.5, QColor(255, 255, 0))   # Yellow
        gradient.setColorAt(1.0, QColor(0, 0, 255))     # Blue

        painter.setBrush(gradient)
        painter.setPen(Qt.black)
        painter.drawRect(20, 10, 20, 200)

        # Draw labels
        font = QFont()
        font.setPointSize(8)
        painter.setFont(font)
        painter.setPen(Qt.black)
        painter.drawText(50, 15, "High")
        painter.drawText(50, 110, "Mid")
        painter.drawText(50, 205, "Low")

        painter.end()

if __name__ == "__main__":
    app = QApplication(sys.argv)

    main = QWidget()
    main.setWindowTitle("Gradient Colorbar Example")
    main.setGeometry(100, 100, 400, 300)

    colorbar = GradientColorbar(main)
    colorbar.move(10, 10)

    main.show()
    sys.exit(app.exec_())



from sortedcontainers import SortedList
from operator import itemgetter
import time

print(["Good", "bad"][input("say smth") == "yes"])
a = [0, 1, 2, 3,4 , 5, 6, 7]
snip = [3, 4, 5]
def test():
    def hello():
        print('hello')

print(a[:len(a)-2])
print(a[len(a)-5:len(a)-3])
print(a[len(a)-len(snip)-2:])
print(a[:6], a[6:])

data = [
    {"a": 1, "b": 2, "c": 3},
    {"a": 4, "b": 5, "c": 6},
    {"a": 7, "b": 8, "c": 9},
]
getter = itemgetter("a", "b")
values = list(map(lambda d: list(getter(d)), data))
print(values)

import dataAPI
import numpy as np

print(np.array(list(enumerate([2, 45, 1, 3]))))
t = round(time.time())
debugPlotData = []


def update():
    tStamp = round(time.time())-t
  
    while (len(debugPlotData) < tStamp+1):
        debugPlotData.append([0, 0])
    debugPlotData[tStamp][0] += 1
    print(debugPlotData[:, 0])

DataManager = dataAPI.DataMain([0, 0], 1)

print("busd", DataManager.DataBase[DataManager.DataBase.bisect_left({"timestamp":100})])
gps = DataManager.extraxtData("gps")
height = DataManager.extraxtData("height")
result = np.column_stack((gps, height))
for i in range(3):
    result[:, i] -= result[len(result)-1][i]
    result[:, :2] /= 100
print(result)


