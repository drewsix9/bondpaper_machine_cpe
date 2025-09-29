#include "CoinCounter.h"

CoinCounter::CoinCounter(uint8_t counterPin, String name)
    : counterButton(counterPin), counterName(name), currentCount(0), 
      targetCount(0), newCoinDetected(false), dispensing(false),
      targetReached(false), timedOut(false), lastCoinTime(0) {}

void CoinCounter::begin() {
    counterButton.setDebounceTime(debounceDelay);
}

void CoinCounter::update() {
    counterButton.loop();
    
    if (counterButton.isPressed()) {
        currentCount++;
        newCoinDetected = true;
        lastCoinTime = millis();
        timedOut = false;
        
        Serial.print(counterName);
        Serial.print(" Count: ");
        Serial.print(currentCount);
        
        if (dispensing && targetCount > 0) {
            Serial.print("/");
            Serial.print(targetCount);
        }
        Serial.println();
        
        // Check if target is reached
        if (dispensing && targetCount > 0 && currentCount >= targetCount) {
            targetReached = true;
            dispensing = false;
            Serial.print(counterName);
            Serial.println(" TARGET REACHED!");
        }
    }
    
    if (dispensing) {
        checkTimeout();
    }
}

void CoinCounter::checkTimeout() {
    if (dispensing && !targetReached && (millis() - lastCoinTime) >= timeoutDuration) {
        timedOut = true;
        dispensing = false;
        Serial.print(counterName);
        Serial.print(" TIMEOUT! Final count: ");
        Serial.print(currentCount);
        Serial.print("/");
        Serial.println(targetCount);
    }
}

void CoinCounter::reset() {
    currentCount = 0;
    newCoinDetected = false;
    targetReached = false;
    timedOut = false;
    dispensing = false;
    targetCount = 0;
    lastCoinTime = 0;
    Serial.print(counterName);
    Serial.println(" Reset");
}

void CoinCounter::dispense(int numberOfCoins) {
    if (numberOfCoins <= 0) {
        Serial.print(counterName);
        Serial.println(" Invalid dispense amount");
        return;
    }
    
    targetCount = numberOfCoins;
    currentCount = 0;
    dispensing = true;
    targetReached = false;
    timedOut = false;
    lastCoinTime = millis();
    
    Serial.print(counterName);
    Serial.print(" Started dispensing ");
    Serial.print(numberOfCoins);
    Serial.println(" coins");
}

bool CoinCounter::hasReachedTarget() {
    return targetReached;
}

bool CoinCounter::hasTimedOut() {
    return timedOut;
}

bool CoinCounter::isDispensing() {
    return dispensing;
}

void CoinCounter::stopDispensing() {
    dispensing = false;
    Serial.print(counterName);
    Serial.println(" Stopped dispensing");
}

int CoinCounter::getCount() {
    return currentCount;
}

String CoinCounter::getName() {
    return counterName;
}

bool CoinCounter::hasNewCoin() {
    if (newCoinDetected) {
        newCoinDetected = false;
        return true;
    }
    return false;
}

void CoinCounter::handleSerial(const String& cmd) {
        
        String prefix = counterName;
        prefix.toLowerCase();
        
        if (cmd == prefix + "_reset") {
            reset();
        } else if (cmd == prefix + "_get") {
            Serial.print(counterName);
            Serial.print(" Count: ");
            Serial.print(currentCount);
            if (dispensing && targetCount > 0) {
                Serial.print("/");
                Serial.print(targetCount);
            }
            Serial.println();
        } else if (cmd.startsWith(prefix + "_dispense_")) {
            int amount = cmd.substring((prefix + "_dispense_").length()).toInt();
            dispense(amount);
        } else if (cmd == prefix + "_stop") {
            stopDispensing();
        }
}