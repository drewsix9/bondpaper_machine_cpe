import time
import RPi.GPIO as gpio
from include.config import a4_step_motors, long_step_motors
import sys

# defines
CW  = 1
CCW = 0

gpio.setmode(gpio.BCM)
class StepperMotor():
    def __init__(self, config = { 'pulse_pin': 0, 'dir_pin': 0 }):
        self.pulse_pin = config['pulse_pin']
        self.dir_pin = config['dir_pin']

        gpio.setmode(gpio.BCM)
        gpio.setup(self.pulse_pin, gpio.OUT)
        gpio.setup(self.dir_pin, gpio.OUT)

    def _stepper_rotate(self, steps, direction, delay=0.001):
        gpio.output(self.dir_pin, gpio.HIGH if direction else gpio.LOW)
        for _ in range(steps):
            #print("step")
            gpio.output(self.pulse_pin, gpio.HIGH)
            time.sleep(delay)  # Pulse duration
            gpio.output(self.pulse_pin, gpio.LOW)
            time.sleep(delay)  # Pulse interval


class PaperDispenser():

    # Default steps values
    stepper_steps = 1000

    # Stepper motor config objects = {pulse_pin, dir_pin, steps}
    def __init__(self, stepper = {}, dc_motor = {}):
        self.stepper = StepperMotor(stepper)
        self.stepper_steps = stepper['steps']

        self.IN1 = dc_motor['IN1']
        self.IN2 = dc_motor['IN2']
        self.ENA = dc_motor['en_pin']

        self.limit_switch = stepper['limit_switch']
        gpio.setup(self.limit_switch, gpio.IN, pull_up_down=gpio.PUD_UP)

        gpio.setup(self.IN1, gpio.OUT)
        gpio.setup(self.IN2, gpio.OUT)
        gpio.setup(self.ENA, gpio.OUT)

        self.pwm = gpio.PWM(self.ENA, 1000)
        self.pwm.start(100)

        gpio.output(self.IN1, gpio.LOW)
        gpio.output(self.IN2, gpio.LOW)

    def ramp_down(self):
        gpio.output(self.IN1, gpio.LOW)
        gpio.output(self.IN2, gpio.HIGH)
        time.sleep(8)
        gpio.output(self.IN1, gpio.LOW)
        gpio.output(self.IN2, gpio.LOW)

    def dispense(self, num_of_papers):
        gpio.output(self.IN1, gpio.HIGH)
        gpio.output(self.IN2, gpio.LOW)
        while not gpio.input(self.limit_switch) == gpio.HIGH:
            print(f"waiting for switch: limit {self.limit_switch}")

        gpio.output(self.IN1, gpio.LOW)
        gpio.output(self.IN2, gpio.LOW)
        print("switch reached")
        for _ in range(num_of_papers):

            # wait to make sure right position
            while not gpio.input(self.limit_switch):
                print("switch not reached")
                pass
            
            self.stepper._stepper_rotate(self.stepper_steps, CW)


n = len(sys.argv)
if n < 4:
    print("Usage: python3 stepper_test <stepper_one_steps> <stepper_two_steps> <number_of_papers>")
    sys.exit()

a4_step_motors['stepper']['steps'] = int(sys.argv[1])
long_step_motors['stepper']['steps'] = int(sys.argv[2])


dispensers = {
    "A4": PaperDispenser(a4_step_motors['stepper'], a4_step_motors['dc_motor']),
    "LONG": PaperDispenser(long_step_motors['stepper'], long_step_motors['dc_motor'])
}

print("Dispensing", int(sys.argv[3]))
dispensers['A4'].dispense(int(sys.argv[3]))
dispensers['A4'].ramp_down()
dispensers['LONG'].dispense(int(sys.argv[3]))
dispensers['LONG'].ramp_down()

