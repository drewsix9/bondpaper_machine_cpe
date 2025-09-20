#pragma once

#include <Arduino.h>

class CoinSensor {
public:
  CoinSensor(uint8_t pin, unsigned long initialCoins);

  void begin();
  void handleSerial();
  bool isDispensing() const;
  unsigned long getCoinsRemaining() const;
  uint8_t getPin() const;

private:
  uint8_t value;
  uint8_t pin;
  volatile long coinsToDispense;
  volatile bool dispensing;

  void coinSensorISR();
};