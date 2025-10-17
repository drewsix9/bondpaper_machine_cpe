import time
import RPi.GPIO as gpio
#import lgpio as gpio

# defines
CW  = 1
CCW = 0

class StepperMotor():
    def __init__(self, config = { 'pulse_pin': 0, 'dir_pin': 0 }):
        self.pulse_pin = config['pulse_pin']
        self.dir_pin = config['dir_pin']

        gpio.setmode(gpio.BCM)
        #gpio.setmode(gpio.BOARD)
        gpio.setup(self.pulse_pin, gpio.OUT)
        gpio.setup(self.dir_pin, gpio.OUT)

    def _stepper_rotate(self, steps, direction, delay=0.001):
        gpio.output(self.dir_pin, gpio.HIGH if direction else gpio.LOW)
        for _ in range(steps):
            gpio.output(self.pulse_pin, gpio.HIGH)
            time.sleep(delay)  # Pulse duration
            gpio.output(self.pulse_pin, gpio.LOW)
            time.sleep(delay)  # Pulse interval
