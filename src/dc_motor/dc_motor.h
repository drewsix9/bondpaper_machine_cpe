#pragma once

#include <Arduino.h>

class DCMotor {
public:
    DCMotor(uint8_t in1Pin, uint8_t in2Pin, uint8_t enablePin, String name = "Motor");
    void begin();
    void forward();
    void backward();
    void stop();
    void setSpeed(uint8_t speed);
    void handleSerial();

private:
    uint8_t in1Pin;
    uint8_t in2Pin;
    uint8_t enablePin;
    String motorName;
    uint8_t currentSpeed;
};
