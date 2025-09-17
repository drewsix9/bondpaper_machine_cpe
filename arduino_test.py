import serial
import time

ser = serial.Serial('/dev/ttyACM0', 9600)
time.sleep(2)

def check_coins():
    ser.write('get\n'.encode())
    res = ser.readline().decode().strip()
    print(res)

if __name__ == '__main__':
    while True:
        check_coins()
        time.sleep(2)

