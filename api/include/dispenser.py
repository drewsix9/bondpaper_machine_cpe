from include.stepper_motor import StepperMotor
from include.stepper_motor import CW, CCW

class PaperDispenser():

    # Default steps values
    stepper_one_steps = 1000
    stepper_two_steps = 1000

    # Stepper motor config objects = {pulse_pin, dir_pin, steps}
    def __init__(self, stepper_one = {}, stepper_two = {}):
        self.stepper_one = StepperMotor(stepper_one)
        self.stepper_one_steps = stepper_one['steps']

        self.stepper_two = StepperMotor(stepper_two)
        self.stepper_two_steps = stepper_two['steps']

    def dispense(self, num_of_papers):
        for _ in range(num_of_papers):
            self.stepper_one._stepper_rotate(self.stepper_one_steps, CW)
            self.stepper_two._stepper_rotate(self.stepper_two_steps, CW)