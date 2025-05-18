import numpy as np
class gradient():
    def __init__(self, maxMin, gradientInfo):
        self.maxMin = maxMin
        self.valueRange = (self.maxMin[1] - self.maxMin[0])
        self.gradientInfo = gradientInfo
    
    def GetValue(self, value):
        return (value-self.maxMin[0])/self.valueRange
    
    def GetColorAtPoint(self, value):
        
        #Transform the value to a new one which lies between 0 and 1
        value = self.GetValue(value)
        
        #Loop trough all the gradient points
        for i in range(len(self.gradientInfo) - 1):
            #Check if the current point lies inside this section of the gradient
            if self.gradientInfo[i]['position'] <= value <= self.gradientInfo[i + 1]['position']:                
                #Get the starting and the ending colors of the gradient
                point1 = self.gradientInfo[i]
                point2 = self.gradientInfo[i + 1]
                
                
                
                #adjust the value to go from 0 to 1
                value = (value - point1['position'])/(point2['position']-point1['position'])
                
                #Create the output for the current color
                interpolatedColor = [0, 0, 0, 1]
                
                #Loop trough the RGB values of the colors
                for x in range(3):
                    #Get the value of the gradient at point 
                    interpolatedColor[x] = point1['color'][x]+ value*(point2['color'][x]- point1['color'][x])#/(abs((value-0.5))+0.5))
                #print(interpolatedColor, (abs(2*(value-0.5))+0.5))
                return interpolatedColor
    
    def SetMaxMin(self, min = None, max = None):
        self.maxMin = (
            [min, self.maxMin[0]][min == None],\
            [max, self.maxMin[1]][max == None])
        
        self.valueRange = (self.maxMin[1] - self.maxMin[0])

    
        
       
            
if __name__ == "__main__":
    import ReadSettings
    from PIL import Image
    import numpy as np
    
    gradInfo = ReadSettings.Settings()["gradient"]
    
    rang = (0, 1000)
    img = np.zeros((10, rang[1], 3), dtype=np.uint8)
    grad = gradient((rang[0], rang[1]), gradInfo)
    for x in range(rang[0], rang[1]):
        img[:, x, :] = grad.GetColorAtPoint(x)
    
    print(grad.GetColorAtPoint(510))
    Image.fromarray(img).show()
    
    
    