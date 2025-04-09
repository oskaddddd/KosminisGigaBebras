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

print(np.array(list(enumerate([2, 45, 1, 3]))))
print(range(5))
DataManager = dataAPI.DataMain([0, 0], 1)

def updateLines():
        gpsGetter = itemgetter("humidity")
        heightGetter = itemgetter("height")
        
        #Extract gps and height data into a np array
        gps = np.array(list(map(gpsGetter, DataManager.DataBase)))
        height = np.array(list(map(heightGetter, DataManager.DataBase)))

        result = np.column_stack((gps, height))

        #Normalise the data
        
            
        print(result)
updateLines()

