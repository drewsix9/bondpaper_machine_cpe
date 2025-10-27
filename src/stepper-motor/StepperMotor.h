#ifndef STEPPERMOTOR_H
#define STEPPERMOTOR_H

#include <Arduino.h>

// Direction constants
#define CW  1
#define CCW 0

class StepperMotor {
public:
    StepperMotor(uint8_t pulsePin, uint8_t dirPin);
    void begin();
    void rotate(int steps, uint8_t direction, unsigned int delayMicros = 1000);
    void setSpeed(unsigned int delayMicros);
    
private:
    uint8_t _pulsePin;
    uint8_t _dirPin;
    unsigned int _stepDelay;
    
    void _step();
};

class PaperDispenser {
public:
    PaperDispenser(uint8_t pulsePin, uint8_t dirPin, uint8_t in1Pin, uint8_t in2Pin, 
                   uint8_t enPin, uint8_t limitSwitchPin, int stepperSteps = 1000);
    void begin();
    void dispense(int numOfPapers);
    void rampDown();
    void setStepperSteps(int steps);
    bool isLimitSwitchPressed();
    
private:
    StepperMotor _stepper;
    uint8_t _in1Pin;
    uint8_t _in2Pin;
    uint8_t _enPin;
    uint8_t _limitSwitchPin;
    int _stepperSteps;
    
    void _dcMotorForward();
    void _dcMotorReverse();
    void _dcMotorStop();
    void _waitForLimitSwitch();
};

#endif