#include "StepperMotor.h"

// StepperMotor Class Implementation
StepperMotor::StepperMotor(uint8_t pulsePin, uint8_t dirPin) {
    _pulsePin = pulsePin;
    _dirPin = dirPin;
    _stepDelay = 1000; // Default 1ms delay
}

void StepperMotor::begin() {
    pinMode(_pulsePin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    digitalWrite(_pulsePin, LOW);
    digitalWrite(_dirPin, LOW);
}

void StepperMotor::rotate(int steps, uint8_t direction, unsigned int delayMicros) {
    // Set direction
    digitalWrite(_dirPin, direction == CW ? HIGH : LOW);
    
    // Perform steps
    for (int i = 0; i < abs(steps); i++) {
        _step();
        delayMicroseconds(delayMicros);
    }
}

void StepperMotor::setSpeed(unsigned int delayMicros) {
    _stepDelay = delayMicros;
}

void StepperMotor::_step() {
    digitalWrite(_pulsePin, HIGH);
    delayMicroseconds(_stepDelay);
    digitalWrite(_pulsePin, LOW);
    delayMicroseconds(_stepDelay);
}

// PaperDispenser Class Implementation
PaperDispenser::PaperDispenser(uint8_t pulsePin, uint8_t dirPin, uint8_t in1Pin, 
                               uint8_t in2Pin, uint8_t enPin, uint8_t limitSwitchPin, 
                               int stepperSteps) 
    : _stepper(pulsePin, dirPin) {
    _in1Pin = in1Pin;
    _in2Pin = in2Pin;
    _enPin = enPin;
    _limitSwitchPin = limitSwitchPin;
    _stepperSteps = stepperSteps;
}

void PaperDispenser::begin() {
    // Initialize stepper motor
    _stepper.begin();
    
    // Initialize DC motor pins
    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);
    pinMode(_enPin, OUTPUT);
    
    // Initialize limit switch
    pinMode(_limitSwitchPin, INPUT_PULLUP);
    
    // Set DC motor to full speed and stop
    analogWrite(_enPin, 255);
    _dcMotorStop();
    
    Serial.println("PaperDispenser initialized");
}

void PaperDispenser::dispense(int numOfPapers) {
    Serial.print("Dispensing ");
    Serial.print(numOfPapers);
    Serial.println(" papers");
    
    // Move to initial position
    _dcMotorForward();
    _waitForLimitSwitch();
    _dcMotorStop();
    
    Serial.println("Initial position reached");
    
    // Dispense each paper
    for (int i = 0; i < numOfPapers; i++) {
        Serial.print("Dispensing paper ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.println(numOfPapers);
        
        // Ensure we're at the limit switch
        if (!isLimitSwitchPressed()) {
            _dcMotorForward();
            _waitForLimitSwitch();
            _dcMotorStop();
        }
        
        // Step the stepper motor to dispense one paper
        _stepper.rotate(_stepperSteps, CW);
        
        delay(100); // Small delay between papers
    }
    
    // Ramp down after dispensing
    rampDown();
    Serial.println("Dispensing complete");
}

void PaperDispenser::rampDown() {
    Serial.println("Ramping down...");
    _dcMotorReverse();
    delay(8000); // 8 second ramp down
    _dcMotorStop();
    Serial.println("Ramp down complete");
}

void PaperDispenser::setStepperSteps(int steps) {
    _stepperSteps = steps;
    Serial.print("Stepper steps set to: ");
    Serial.println(steps);
}

bool PaperDispenser::isLimitSwitchPressed() {
    return digitalRead(_limitSwitchPin) == HIGH;
}

void PaperDispenser::_dcMotorForward() {
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, LOW);
}

void PaperDispenser::_dcMotorReverse() {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, HIGH);
}

void PaperDispenser::_dcMotorStop() {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
}

void PaperDispenser::_waitForLimitSwitch() {
    while (!isLimitSwitchPressed()) {
        Serial.print("Waiting for limit switch on pin ");
        Serial.println(_limitSwitchPin);
        delay(100);
    }
}