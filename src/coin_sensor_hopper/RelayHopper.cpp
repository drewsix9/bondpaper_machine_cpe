#include "RelayHopper.h"

RelayController::RelayController(uint8_t pin1, uint8_t pin2, uint8_t pin3) {
    relayPins[0] = pin1;
    relayPins[1] = pin2;
    relayPins[2] = pin3;
}

void RelayController::begin() {
    for (int i = 0; i < 3; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], HIGH);
    }
}

void RelayController::setRelay(uint8_t relayNum, bool state) {
    if (relayNum >= 1 && relayNum <= 3) {
        digitalWrite(relayPins[relayNum - 1], state ? HIGH : LOW);
        Serial.print("Relay ");
        Serial.print(relayNum);
        Serial.println(state ? " ON" : " OFF");
    }
}

void RelayController::handleSerial() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "relay1_on") setRelay(1, false);
        else if (cmd == "relay1_off") setRelay(1, true);
        else if (cmd == "relay2_on") setRelay(2, false);
        else if (cmd == "relay2_off") setRelay(2, true);
        else if (cmd == "relay3_on") setRelay(3, false);
        else if (cmd == "relay3_off") setRelay(3, true);
    }
}