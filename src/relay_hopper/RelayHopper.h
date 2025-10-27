#pragma once

#include <Arduino.h>

class RelayController {
public:
    RelayController(uint8_t pin1, uint8_t pin2, uint8_t pin3);
    void begin();
    void setRelay(uint8_t relayNum, bool state);
    void handleSerial();

private:
    uint8_t relayPins[3];
};
