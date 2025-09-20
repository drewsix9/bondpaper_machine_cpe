#pragma once

#include <Arduino.h>

class CoinSlotISR {
public:
    CoinSlotISR(uint8_t pulsePin);
    void begin();
    void handle();
    void handleSerial();
    static void coinPulseISR();

    int getCoinValue(int count);
    void processCommand(String cmd);
private:
    uint8_t pulsePin;
    static volatile unsigned long pulseCount;
    static volatile unsigned long lastPulseTime;
    static volatile bool coinSlotStatus;
    static volatile int totalValue;
    String serialCommand;
    static const unsigned long coinDetectTimeout = 300;
    static const unsigned long coinDebounceTime = 50;
    
    void processCoin(int count);
    
};
