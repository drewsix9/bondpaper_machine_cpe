#include "DCMotor.h"

DCMotor::DCMotor(uint8_t in1Pin, uint8_t in2Pin, uint8_t enablePin, String name)
    : in1Pin(in1Pin), in2Pin(in2Pin), enablePin(enablePin), motorName(name), currentSpeed(255) {}

void DCMotor::begin() {
    pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);
    pinMode(enablePin, OUTPUT);
    stop();
}

void DCMotor::forward() {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    analogWrite(enablePin, currentSpeed);
    Serial.println(motorName + " Forward");
}

void DCMotor::backward() {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    analogWrite(enablePin, currentSpeed);
    Serial.println(motorName + " Backward");
}

void DCMotor::stop() {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    digitalWrite(enablePin, LOW);
    Serial.println(motorName + " Stopped");
}

void DCMotor::setSpeed(uint8_t speed) {
    currentSpeed = speed;
}

void DCMotor::handleSerial() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        String prefix = motorName.toLowerCase();
        
        if (cmd == prefix + "_forward") {
            forward();
        } else if (cmd == prefix + "_backward") {
            backward();
        } else if (cmd == prefix + "_stop") {
            stop();
        }
    }
}