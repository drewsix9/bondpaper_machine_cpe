#include "coin_sensor.h"

CoinSensor* CoinSensor::instance = nullptr;

CoinSensor::CoinSensor(uint8_t pin, uint8_t value)
    : value(value), pin(pin), coinsToDispense(0), dispensing(true) {}

void CoinSensor::begin() {
  pinMode(pin, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(pin), coinSensorISR, RISING);
}

void CoinSensor::handleSerial() {
  if (!dispensing) {
    // Talk to Serial for communication
    Serial.print("Complete ");
    Serial.println(value);
  }
}

bool CoinSensor::isDispensing() const {
  return dispensing;
}

unsigned long CoinSensor::getCoinsRemaining() const {
  return coinsToDispense;
}

uint8_t CoinSensor::getPin() const {
  return pin;
}

void CoinSensor::isrRouter() {
  if (instance != nullptr) {
    instance->coinSensorISR();
  }
}

void CoinSensor::coinSensorISR() {
  coinsToDispense--;
  if (coinsToDispense <= 0) {
    coinsToDispense = 0;
    dispensing = false;
  }
}