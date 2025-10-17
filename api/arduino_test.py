import serial
import threading

# Update this to match your port
SERIAL_PORT = '/dev/ttyACM0'  # Windows: 'COM3', Linux: '/dev/ttyUSB0' or '/dev/ttyACM0'
BAUD_RATE = 9600

def read_from_serial(ser):
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[Arduino] {line}")

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return

    # Start thread to read from serial
    thread = threading.Thread(target=read_from_serial, args=(ser,), daemon=True)
    thread.start()

    try:
        while True:
            msg = input()  # Read user input
            if msg.lower() == "exit":
                break
            ser.write((msg + '\n').encode())  # Send message to Arduino
    except KeyboardInterrupt:
        print("\nExiting...")

    ser.close()

if __name__ == '__main__':
    print("Start")
    main()
