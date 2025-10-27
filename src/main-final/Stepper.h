// Stepper.h
#pragma once
#include <Arduino.h>

class Stepper {
  public:
    void begin(uint8_t pinPulse, uint8_t pinDir, uint16_t stepUs);
    void rotate(long n, bool dirCW);

  private:
    uint8_t pPulse_, pDir_;
    uint16_t stepUs_;
};
