// Stepper.cpp
#include "Stepper.h"

void Stepper::begin(uint8_t pinPulse, uint8_t pinDir, uint16_t stepUs) {
  pPulse_ = pinPulse; pDir_ = pinDir; stepUs_ = stepUs;
  pinMode(pPulse_, OUTPUT);
  pinMode(pDir_, OUTPUT);
  digitalWrite(pPulse_, LOW);
  digitalWrite(pDir_, LOW); // WAS NOT HERE BEFORE
}

void Stepper::rotate(long n, bool dirCW) {
  digitalWrite(pDir_, dirCW ? HIGH : LOW);
  for (long i=0; i<n; ++i) {
    digitalWrite(pPulse_, HIGH);
    delayMicroseconds(stepUs_);
    digitalWrite(pPulse_, LOW);
    delayMicroseconds(stepUs_);
  }
}
