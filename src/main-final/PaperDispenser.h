// PaperDispenser.h
#pragma once
#include <Arduino.h>
#include "Stepper.h"

class PaperDispenser {
  public:
    void begin(uint8_t in1, uint8_t in2, uint8_t enPWM,
               uint8_t limitPin,
               uint8_t pulsePin, uint8_t dirPin, uint16_t stepUs,
               long stepsPerSheet,
               uint8_t paperPresencePin);

    void dispenseSheets(uint16_t count);
    
    // Paper presence detection
    bool hasPaper(bool printStatus = false);

  private:
    uint8_t in1_, in2_, en_, limit_;
    uint8_t paperPresencePin_;
    Stepper stepper_;
    long stepsPerSheet_;
    void dcForward(bool on);
    void dcReverse(bool on);
    void homeToLimit();
    void rampDown();
    bool paperPresent(uint8_t pin);
    bool noPaperOrFault(uint8_t pin);
};
