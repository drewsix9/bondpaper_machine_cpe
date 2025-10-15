// PaperDispenser.cpp
#include "PaperDispenser.h"
#include "config.h"

void PaperDispenser::begin(uint8_t in1, uint8_t in2, uint8_t enPWM,
                           uint8_t limitPin,
                           uint8_t pulsePin, uint8_t dirPin, uint16_t stepUs,
                           long stepsPerSheet) {
  in1_ = in1; in2_ = in2; en_ = enPWM; limit_ = limitPin;
  pinMode(in1_, OUTPUT);
  pinMode(in2_, OUTPUT);
  pinMode(en_,  OUTPUT);
  pinMode(limit_, INPUT_PULLUP);
  analogWrite(en_, 255); // ~80% (0..255) — tune as needed
  dcForward(false);

  stepper_.begin(pulsePin, dirPin, stepUs);
  stepsPerSheet_ = stepsPerSheet;
}

void PaperDispenser::dcForward(bool on) {
  digitalWrite(in1_, on ? HIGH : LOW);
  digitalWrite(in2_, LOW);
}

void PaperDispenser::dcReverse(bool on) {
  digitalWrite(in1_, LOW);
  digitalWrite(in2_, on ? HIGH : LOW);
}

void PaperDispenser::homeToLimit() {
  // Move forward until limit switch reads HIGH
  dcForward(true);
  const uint32_t timeout = millis() + 10000; // safety
  while (digitalRead(limit_) != HIGH && millis() < timeout) { /*spin*/ }
  dcForward(false);
}

void PaperDispenser::rampDown() {
  dcReverse(true);
  delay(RAMP_DOWN_MS);
  dcReverse(false);
}

void PaperDispenser::dispenseSheets(uint16_t count) {
  // Ensure we are at a good starting pose
  homeToLimit();

  for (uint16_t i=0; i<count; ++i) {
    // If limit released (LOW), nudge forward until it's HIGH again
    if (digitalRead(limit_) != HIGH) dcForward(true);
    const uint32_t timeout = millis() + 8000;
    while (digitalRead(limit_) != HIGH && millis() < timeout) { /*spin*/ }
    dcForward(false);

    // Feed one sheet
    stepper_.rotate(stepsPerSheet_, DIR_CW);
    delay(100);
  }

  rampDown();
}
