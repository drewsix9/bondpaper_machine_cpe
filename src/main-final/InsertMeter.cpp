// InsertMeter.cpp
#include "InsertMeter.h"
#include "config.h"

void InsertMeter::begin(uint8_t pulsePin, uint8_t intMode) {
  pinMode(pulsePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pulsePin), []{ /* filled in setup */ }, intMode);
}

void InsertMeter::isr() {
  // Use debounce similar to CoinSlotISR
  unsigned long now = millis();
  // Only count a pulse if it's been at least 50ms since the last one
  if (now - lastMs_ > COIN_BURST_TIMEOUT_MS) {
    pulses_++;
    lastMs_ = now;
  }
}

void InsertMeter::loop() {
  
  // When pulses have stopped for COIN_BURST_TIMEOUT_MS, finalize as one coin
  if (pulses_ > 0 && (millis() - lastMs_ > 300)) {
    classify((int)pulses_);
    pulses_ = 0;  // Reset pulse counter for next coin
  }
}


void InsertMeter::classify(int p) {
  int coinValue = 0;
  
  // Use exact matching like in CoinSlotISR implementation
  switch (p) {
    case 1: coinValue = 1; break;
    case 3: coinValue = 5; break;
    case 6: coinValue = 10; break;
    case 9: coinValue = 20; break;
  }
  
  // Add the coin value to the total
  total_ += coinValue;
}
