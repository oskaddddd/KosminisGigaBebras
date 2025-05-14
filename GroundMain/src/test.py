from sortedcontainers import SortedList
from operator import itemgetter
import time
a = [0, 1, 2, 3,4 , 5, 6, 7]
snip = [3, 4, 5]
print(a[0:-2], a[1:-2+1])
print(a[2:], a[2-1:-1])
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


