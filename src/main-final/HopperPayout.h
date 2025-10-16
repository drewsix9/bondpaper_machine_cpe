#pragma once
#include <Arduino.h>
#include <ezButton.h>
#include "config.h"   // for HOPPER_MAX_GAP_MS (add there if you want to tune it)

struct HopperPins {
  uint8_t actuatorPin;   // relay/MOSFET
  uint8_t sensorPin;     // ezButton sensor input
  bool    activeLow;     // true for typical relay boards
};

class HopperPayout {
public:
  void begin(uint8_t denom, const HopperPins& pins, uint16_t debounceMs = 10);
  bool start(uint16_t n);   // returns false if already busy
  void stop();              // abort (no DONE line)
  void reset();             // reset all counters and state
  void loop();              // call in main loop()
  bool busy() const;

private:
  void on_();
  void off_();

  ezButton* btn_ = nullptr;
  HopperPins pins_{};
  uint8_t   denom_ = 0;

  uint16_t  target_ = 0, count_ = 0;
  bool      busy_ = false;
  uint32_t  lastCoin_ = 0;
  
  // Add missing member variables
  bool      pulsing_ = false;
  uint32_t  pulseStart_ = 0;
  uint32_t  coolStart_ = 0;
};
