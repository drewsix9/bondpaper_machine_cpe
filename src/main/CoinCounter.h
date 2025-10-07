#ifndef COINCOUNTER_H
#define COINCOUNTER_H

#include <Arduino.h>
#include <ezButton.h>

class CoinCounter {
public:
    CoinCounter(uint8_t counterPin, String name = "Counter");
    void begin();
    void update();
    void reset();
    int getCount();
    String getName();
    bool hasNewCoin();
    void handleSerial(const String& cmd);
    
    // Dispense function
    void dispense(int numberOfCoins);
    bool hasReachedTarget();
    bool hasTimedOut();
    bool isDispensing();
    void stopDispensing();

private:
    ezButton counterButton;
    String counterName;
    int currentCount;
    int targetCount;
    bool newCoinDetected;
    bool dispensing;
    bool targetReached;
    bool timedOut;
    unsigned long lastCoinTime;
    const unsigned long debounceDelay = 10;
    const unsigned long timeoutDuration = 3000; // Fixed 3 seconds
    
    void checkTimeout();
};

#endif // COINCOUNTER_H