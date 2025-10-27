#include <Wire.h>

#define COINS_PULSE_PIN 2
#define BUZZER_PIN 12

volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
const unsigned long coinDetectTimeout = 300; 
const unsigned long coinDebounceTime = 50;

// State 
volatile bool coinSlotStatus = false;
volatile int totalValue = 0;
String serialCommand = "";

void setup() {
  Serial.begin(115200);
  pinMode(COINS_PULSE_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(COINS_PULSE_PIN), coinPulseISR, FALLING);
}

void loop() {

  if(coinSlotStatus && (millis() - lastPulseTime > coinDetectTimeout)) {
    processCoin(pulseCount);
    pulseCount = 0;
    coinSlotStatus = false;
  }

  while(Serial.available() > 0) {
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

int getCoinValue(int count) {
  switch (count) {
    case 1: return 1;
    case 3: return 5;
    case 6: return 10;
    case 9: return 20;
    default: return 0;
  }
}

void coinPulseISR() {
  unsigned long now = millis();
  if (now - lastPulseTime > coinDebounceTime) {
    pulseCount++;
    lastPulseTime = now;
    coinSlotStatus = true;
  }
}

void processCoin(int count){
  int value = getCoinValue(count);
  totalValue += value;
  Serial.println(value);
}

void processCommand(String cmd) {
  if (cmd == "get") {
    Serial.println(totalValue);
  } else if (cmd == "reset") {
    totalValue = 0;
    Serial.println(0);
  } else {
    Serial.println("Unknown command");
  }
}

