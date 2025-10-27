#include "CoinSlotISR.h"

#define COINS_PULSE_PIN 2
#define BUZZER_PIN 12

CoinSlotISR coinSlot(COINS_PULSE_PIN, BUZZER_PIN);

void setup() {
    coinSlot.begin();
}

void loop() {
    coinSlot.handle();
    coinSlot.handleSerial();
}