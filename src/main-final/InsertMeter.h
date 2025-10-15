// InsertMeter.h
#pragma once
#include <Arduino.h>

class InsertMeter {
  public:
    void begin(uint8_t pulsePin, uint8_t intMode);
    void isr();
    void loop();
    int total() const { return total_; }
    void reset() { total_ = 0; pulses_ = 0; }

  private:
    volatile uint16_t pulses_ = 0;
    volatile uint32_t lastMs_ = 0;
    volatile int total_ = 0;
    void classify(int p);
};
