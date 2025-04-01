
import dataAPI
from PyQt6.QtGui import QVector3D, QSurfaceFormat
from PyQt6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget, QLabel
from PyQt6.QtCore import QTimer
import pyqtgraph.opengl as gl
import numpy as np
import sys
from collections import OrderedDict

# Optionally set a specific OpenGL version:
fmt = QSurfaceFormat()
fmt.setVersion(2, 1)  # Try with OpenGL 2.1
QSurfaceFormat.setDefaultFormat(fmt)


class MyGLViewWidget(gl.GLViewWidget):
    def paintGL(self):
        self.makeCurrent()
        super().paintGL()

class FlightPathVisualizer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Flight Path Visualizer")
        self.setMinimumSize(1000, 1000)  # Use a stable, conventional size
        self.gps_data = OrderedDict()

        try:
            self.gl_widget = MyGLViewWidget()  # Use subclassed widget
            self.gl_widget.setCameraPosition(distance=200)
            self.gl_widget.opts['center'] = QVector3D(0, 0, 0)
            self.line_item = gl.GLLinePlotItem()
            self.gl_widget.addItem(self.line_item)
        except Exception as e:
            self.gl_widget = QLabel(f"OpenGL failed to load: {e}")

        container = QWidget()
        layout = QVBoxLayout()
        layout.addWidget(self.gl_widget)
        container.setLayout(layout)
        self.setCentralWidget(container)

        grid = gl.GLGridItem()
        grid.setSize(800, 800)
        grid.setSpacing(10, 10)
        self.gl_widget.addItem(grid)

        # Delay the initial update to allow the GL context to settle
        QTimer.singleShot(1000, self.update_path)

    def add_gps_point(self, timestamp, gps_dict):
        self.gps_data[timestamp] = gps_dict
        self.update_path()

    def update_path(self):
        sorted_items = sorted(self.gps_data.items())
        points = np.array([[v['lon'], v['lat'], v['height']] for _, v in sorted_items], dtype=np.float32)
        if len(points) > 1:
            self.line_item.setData(pos=points, width=2, antialias=False)
        if len(points) > 0:
            center_arr = points.mean(axis=0)
            center = QVector3D(center_arr[0], center_arr[1], center_arr[2])
            self.gl_widget.opts['center'] = center
        self.gl_widget.update()

if __name__ == '__main__':
    import time
    app = QApplication(sys.argv)
    viz = FlightPathVisualizer()
    viz.show()



    for point in DataManager.DataBase:
        viz.add_gps_point(point['timestamp'], {
            'lat': point['lat'],
            'lon': point['lon'],
            'height': point['height']
        })
        app.processEvents()
        time.sleep(0.3)

    sys.exit(app.exec())