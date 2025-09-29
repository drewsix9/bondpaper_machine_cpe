#include "RelayHopper.h"
#include "config.h"

RelayController relays(RELAY1_PIN, RELAY2_PIN, RELAY3_PIN);

// State variables
volatile int countCoin = 0;
volatile bool isDispensing = false;
volatile unsigned long lastCoinTime = 0;

// Timing intervals
const unsigned long coinDebounceTime = 50;    // Debounce time for coin sensors
const unsigned long printInterval = 1000;     // How often to print status while dispensing
const unsigned long timeoutInterval = 2000;   // Timeout after last coin detected

// Timing variables
unsigned long lastPrintTime = 0;
unsigned long lastStatusTime = 0;

void setup() {
  pinMode(ONE_SENSOR_PIN, INPUT_PULLUP);
  pinMode(FIVE_SENSOR_PIN, INPUT_PULLUP);
  pinMode(TEN_SENSOR_PIN, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(ONE_SENSOR_PIN), oneSensorISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(FIVE_SENSOR_PIN), fiveSensorISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(TEN_SENSOR_PIN), tenSensorISR, FALLING);
  
  Serial.begin(115200);
  relays.begin();
  
  Serial.println("Coin sensor system initialized");
}

void processCoin(int count) {
  Serial.print("Final coin count: ");
  Serial.println(count);
  Serial.println("Dispensing complete");
}

void handle() {
  unsigned long now = millis();
  
  // Check if we're in dispensing state
  if (isDispensing) {
    // Print status at regular intervals while dispensing
    if (now - lastStatusTime >= printInterval) {
      Serial.print("Dispensing... Current count: ");
      Serial.println(countCoin);
      lastStatusTime = now;
    }
    
    // Check for timeout (no new coins for timeoutInterval)
    if (now - lastCoinTime >= timeoutInterval) {
      processCoin(countCoin);
      countCoin = 0;
      isDispensing = false;
      Serial.println("Ready for next coins");
    }
  }
}

void oneSensorISR() {
  unsigned long now = millis();
  if (now - lastPrintTime > coinDebounceTime) {
    countCoin++;
    lastPrintTime = now;
    lastCoinTime = now;
    if (!isDispensing) {
      isDispensing = true;
      Serial.println("Started dispensing coins");
      lastStatusTime = now;
    }
  }
}

void fiveSensorISR() {
  unsigned long now = millis();
  if (now - lastPrintTime > coinDebounceTime) {
    countCoin += 5;
    lastPrintTime = now;
    lastCoinTime = now;
    if (!isDispensing) {
      isDispensing = true;
      Serial.println("Started dispensing coins");
      lastStatusTime = now;
    }
  }
}

void tenSensorISR() {
  unsigned long now = millis();
  if (now - lastPrintTime > coinDebounceTime) {
    countCoin += 10;
    lastPrintTime = now;
    lastCoinTime = now;
    if (!isDispensing) {
      isDispensing = true;
      Serial.println("Started dispensing coins");
      lastStatusTime = now;
    }
  }
}

void loop() {
  relays.handleSerial();
  handle();
}