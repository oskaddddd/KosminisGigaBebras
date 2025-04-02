from sortedcontainers import SortedList
from operator import itemgetter
a = [0, 1, 2, 3,4 , 5, 6, 7]
snip = [3, 4, 5]
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
DataManager = dataAPI.DataMain([0, 0], 1)
def updateLines():
        gpsGetter = itemgetter("gps")
        heightGetter = itemgetter("height")
        
        #Extract gps and height data into a np array
        gps = np.array(list(map(list, map(gpsGetter, DataManager.DataBase))))
        print(gps)
        height = np.array(list(map(list, map(heightGetter, DataManager.DataBase))))
        
        #gps = np.array(map(lambda d: gpsGetter(d), DataManager.DataBase))
        #height = np.array(map(lambda d: heightGetter(d), DataManager.DataBase))
        result = np.column_stack((gps, height))
        print(result)
        #Normalise the data
        for i in range(3):
            result[:, i] -= result[0][i]
            
        print(result)
updateLines()