#include "CoinSlotISR.h"

volatile unsigned long CoinSlotISR::pulseCount = 0;
volatile unsigned long CoinSlotISR::lastPulseTime = 0;
volatile bool CoinSlotISR::coinSlotStatus = false;
volatile int CoinSlotISR::totalValue = 0;

CoinSlotISR::CoinSlotISR(uint8_t pulsePin)
    : pulsePin(pulsePin), serialCommand("") {}

void CoinSlotISR::begin() {
  pinMode(pulsePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pulsePin), CoinSlotISR::coinPulseISR, FALLING);
}

void CoinSlotISR::handle() {
  if (coinSlotStatus && (millis() - lastPulseTime > coinDetectTimeout)) {
    processCoin(pulseCount);
    pulseCount = 0;
    coinSlotStatus = false;
  }
}

void CoinSlotISR::handleSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n') {
      serialCommand.trim();
      processCommand(serialCommand);
      serialCommand = "";
    } else {
      serialCommand += c;
    }
  }
}

void CoinSlotISR::coinPulseISR() {
  unsigned long now = millis();
  if (now - lastPulseTime > CoinSlotISR::coinDebounceTime) {
    pulseCount++;
    lastPulseTime = now;
    coinSlotStatus = true;
  }
}

int CoinSlotISR::getCoinValue(int count) {
  switch (count) {
  case 1:
    return 1;
  case 3:
    return 5;
  case 6:
    return 10;
  case 9:
    return 20;
  default:
    return 0;
  }
}

void CoinSlotISR::processCoin(int count) {
  int value = getCoinValue(count);
  totalValue += value;
  Serial.println(value);
}

void CoinSlotISR::processCommand(String cmd) {
  if (cmd == "get") {
    Serial.println(totalValue);
  } else if (cmd == "reset") {
    totalValue = 0;
    Serial.println(0);
  } else {
    Serial.println("Unknown command");
  }
}