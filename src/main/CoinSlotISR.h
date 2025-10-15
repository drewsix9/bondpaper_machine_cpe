#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SerialComm.h"

/**
 * CoinSlotISR - Handles coin acceptor input with Interrupt Service Routines
 * 
 * JSON Command Structure (Raspberry Pi → Arduino):
 * {
 *   "v": 1,                     // Protocol version
 *   "target": "CoinSlot",       // Must be "CoinSlot" for this module
 *   "cmd": "get"                // Command: "get" or "reset"
 * }
 * 
 * Example commands:
 * {"v":1,"target":"CoinSlot","cmd":"get"}
 * {"v":1,"target":"CoinSlot","cmd":"reset"}
 */
class CoinSlotISR {
public:
    explicit CoinSlotISR(uint8_t pulsePin);

    void begin();                         // initialize and attach ISR
    void detach();                        // detach interrupt handler
    bool reattach();                      // reattach interrupt handler, returns true if newly attached
    void update();                        // finalize coin after quiet window

    // String-based API (expects one complete line terminated by '\n')
    // Legacy method - will be removed after refactoring
    void handleSerial(const String& line);
    
    // New method to process commands
    bool processCommand(const String& cmd, JsonDocument& doc);

    static void coinPulseISR();           // ISR: updates s_* counters/flags only
    
    // ==== State tracking ====
    bool isAttached() const { return m_bIsAttached; }  // Check if interrupt is attached - now public
    int getTotalValue() const { return s_nTotalValue; }
    
    // Status reporting methods
    bool hasStatusUpdate() const { return m_bHasStatusUpdate; }
    void clearStatusUpdate() { m_bHasStatusUpdate = false; }
    bool hasEvent() const { return m_bHasEvent; }
    void clearEvent() { m_bHasEvent = false; }
    int getLastCoinValue() const { return m_nLastCoinValue; }

private:
    // ==== Hardware ====
    uint8_t m_u8PulsePin;
    bool m_bIsAttached;                  // Tracks if interrupt is attached

    // ==== ISR-shared state (static & volatile) ====
    static volatile unsigned long s_ulPulseCount;
    static volatile unsigned long s_ulLastPulseTime;
    static volatile bool         s_bCoinActive;
    static volatile int          s_nTotalValue;
    
    // Status reporting flags
    bool m_bHasStatusUpdate;              // Flag: has new status to report
    bool m_bHasEvent;                     // Flag: has new event to report
    int m_nLastCoinValue;                 // Value of last detected coin

    // ==== Timing (ms) ====
    static const unsigned long k_ulCoinDetectTimeout = 300;  // group pulses as one coin
    static const unsigned long k_ulCoinDebounceTime  = 50;   // debounce between pulses

    // ==== Helpers ====
    static int  getCoinValue(int count);
    static void finalizeCoin(int count);

    bool processJsonCommand(JsonDocument& doc);
    void processLegacyCommand(const String& cmd);
};
