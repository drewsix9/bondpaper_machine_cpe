#include "RelayHopper.h"

#define RELAY1_PIN 4
#define RELAY2_PIN 5
#define RELAY3_PIN 6

RelayController relays(RELAY1_PIN, RELAY2_PIN, RELAY3_PIN);

void setup() {
    Serial.begin(115200);
    relays.begin();
}

void loop() {
    relays.handleSerial();
}