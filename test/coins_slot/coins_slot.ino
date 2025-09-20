#include <Wire.h>

#define COINS_SLOT_COUNTER_PIN 2
#define BUZZER_PIN 12

volatile int coinSlotPulse = 0;
volatile int coinSlotStatus;

void setup() {
  Serial.begin(115200);
  pinMode(COINS_SLOT_COUNTER_PIN, INPUT_PULLUP);
}

void loop() {
  coinSlotStatus = digitalRead(COINS_SLOT_COUNTER_PIN);
  delay(30);
  if (coinSlotStatus == 0) {
    coinSlotPulse++;
    Serial.print("Coin detected! Total coins: ");
    Serial.println(coinSlotPulse);
    delay(30);
  }
}
