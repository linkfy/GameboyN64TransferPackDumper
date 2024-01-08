import serial
import time

# The port should be set to your own device
arduino = serial.Serial(port='/dev/ttyACM0', baudrate=9600, timeout=.1)

def read_arduino():
    data = arduino.readline()
    return data

with open('dump.txt', 'w') as f:
    while True:
        data = read_arduino()
        if(data):
            try:
                print(data.decode('utf-8'))
                f.write(data.decode('utf-8'))
            except Exception as e:
               print(e) 
# Press Ctrl+C to finish dumping
