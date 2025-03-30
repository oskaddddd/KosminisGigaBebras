from serial import Serial
from serial.tools import list_ports as list_ports
import time
import queue
send_queue = queue.Queue()
import threading
#Find available ports and promt user to choose
ports = list_ports.comports()
mode = int(input("Select mode\n 0 - communicate, 1 - setup radio, 2 -local chan, 3 - remote chan\nB====D~:"))
print("\nChoose port:")
nports = []
for i, port in enumerate(ports):
    if port.description!="n/a":
        print(f"({port.description})")
        print(f"-{i}- {port.name} {port.description}")
        nports.append(port)



ser1 = Serial(f"/dev/{ports[int(input('Enter1:'))].name if len(nports) != 1 else nports[0].name}", 57600)
if not mode: ser2 = Serial(f"/dev/{ports[int(input('Enter2:'))].name}", 57600)


#Wait till serial initialises
i = 0
while (not ser1.is_open and (not ser2.is_open or not mode)):
    print("Waiting for serial to begin:", i)
    i+=1
    time.sleep(1)

print("Serial initialised!")


#0001000000100000001100000100000001010000011000000111000010000000100100001010
#0110100001000000000000000110100001100001100010011110

def setChanel(NetId):
    
    def wait(ogMessage = ""):
        while ser1.in_waiting <= len(ogMessage):
            time.sleep(0.1)
            print(ogMessage, "wawa", ser1.in_waiting)
            
    ser1.write(b"+++")
    wait("+++")
    
    output = ser1.read_all()
    if b'OK' in output:
        ser1.write(bytes(f'ATS3={NetId}\r\n', 'utf-8'))
    else: return "Failed1"
    
    
    wait(f'ATS3={NetId}\r\n')
            
    output = ser1.read_all()
    if b'OK' in output:
        ser1.write(bytes(f'AT&W\r\n', 'utf-8'))
    else: return "Failed2"
        
    wait('AT&W\r\n')
    
    if b'OK' in output:
        ser1.write(b'ATZ\r\n')

    else: return "Failed3"
    
    
    while (not ser1.is_open):
        print("Waiting for serial to begin:")
        time.sleep(0.2)
    
    print("Serial initialised!")
    
    setupRadio()
    
def remoteNetId(NetId):
    def wait(ogMessage = ""):
        while ser1.in_waiting <= len(ogMessage):
            time.sleep(0.1)
            print(ogMessage, "wawa", ser1.in_waiting)
            
    ser1.write(b"+++")
    wait("+++")
    
    output = ser1.read_all()
    if b'OK' in output:
        ser1.write(bytes(f'RTS3={NetId}\r\n', 'utf-8'))
    else: return "Failed1"
    
    
    wait(f'RTS3={NetId}\r\n')
            
    output = ser1.read_all()
    if b'OK' in output:
        ser1.write(bytes(f'RT&W\r\n', 'utf-8'))
    else: return "Failed2" + output.decode()
        
    wait('RT&W\r\n')
    
    if b'OK' in output:
        ser1.write(b'RTZ\r\n')

    else: return "Failed3"
    time.sleep(1)
            
    output = ser1.read_all()
    if b'OK' in output:
        ser1.write(bytes(f'ATS3={NetId}\r\n', 'utf-8'))
    else: return "Failed4"
    
    
    wait(f'ATS3={NetId}\r\n')
            
    output = ser1.read_all()
    if b'OK' in output:
        ser1.write(bytes(f'AT&W\r\n', 'utf-8'))
    else: return "Failed5"
        
    wait('AT&W\r\n')
    
    if b'OK' in output:
        ser1.write(b'ATZ\r\n')

    else: return "Failed6"
    
    
    while (not ser1.is_open):
        print("Waiting for serial to begin:")
        time.sleep(0.2)
    
    print("Serial initialised!")
    
    setupRadio()
    
    
    
    
    
    
def setupRadio():
    
    
    def send():

        
        
        while True:
            if not send_queue.empty():
                command = send_queue.get()  # Get command from queue
                if command:
                    #print(command)
                    if command not in  [b'+++']:
                        ser1.write(command + (b'\r\n'))
                    else: ser1.write(command)
                    #print(f"Sent: {command.decode('utf-8')}")
            time.sleep(1) 
            if ser1.in_waiting > 0:
                read = ser1.read_all()
                print("\nrecieved message:", read)
                
    def input_thread():
        while True:
            command = input("\nEnter command: ")
            if command:
                send_queue.put(bytes(command, 'utf-8'))
             # Short sleep to prevent 100% CPU usage
    thread1 = threading.Thread(target=send)
    #thread2 = threading.Thread(target=listen)
    thread3 = threading.Thread(target=input_thread)

    # Start the threads
    thread1.start()
    #thread2.start()
    thread3.start()
    
    
             
             
    
    

def communicate():
    def send():
        while True:
            if not send_queue.empty():
                command = send_queue.get()  # Get command from queue
                if command:
                    ser1.write(command)
                    #print(f"Sent: {command.decode('utf-8')}")
                time.sleep(1) 
                if ser2.in_waiting > 0:
                    read = ser2.read_all()
                    print("\SER2 message:", read)
                if ser1.in_waiting > len(command):
                    read = ser1.read_all()
                    print("\SER1 message:", read)
                 # Short sleep to prevent 100% CPU usage

    # Function to listen for incoming data
    def listen():
        while True:
            if ser2.in_waiting > 0:
                read = ser2.read_all()
                print(read)
            time.sleep(1)  # Short sleep to avoid busy-wait

    # Non-blocking input handler
    def input_thread():
        while True:
            command = input("\nEnter command: ")
            if command:
                send_queue.put(bytes(command+("\r\n") if command !="+++" else "", 'utf-8'))  # Put the command in the queue

    # Create threads for sending, listening, and input handling
    thread1 = threading.Thread(target=send)
    #thread2 = threading.Thread(target=listen)
    thread3 = threading.Thread(target=input_thread)

    # Start the threads
    thread1.start()
    #thread2.start()
    thread3.start()

if mode == 2: print(setChanel(int(input("Enter address")))) 
elif mode == 3: print(remoteNetId(int(input("Enter address"))))

elif mode: setupRadio()
else: communicate()
