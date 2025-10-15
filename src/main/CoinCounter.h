#ifndef COINCOUNTER_H
#define COINCOUNTER_H

#include <Arduino.h>
#include <ezButton.h>
#include <ArduinoJson.h>
#include "RelayHopper.h"

/**
 * CoinCounter - Controls and monitors coin counting mechanism
 * Also controls the relay for the coin hopper associated with this counter
 * 
 * JSON Command Structure (Raspberry Pi → Arduino):
 * {
 *   "v": 1,                     // Protocol version
 *   "target": "CoinCounter",    // Must be "CoinCounter" for this module
 *   "cmd": "dispense",          // Command: "reset", "get", "dispense", "stop"
 *   "name": "Counter",          // Name of the counter instance to target
 *   "value": 10                 // Required for dispense command (number of coins)
 * }
 * 
 * Example commands:
 * {"v":1,"target":"CoinCounter","cmd":"dispense","name":"Counter","value":10}
 * {"v":1,"target":"CoinCounter","cmd":"reset","name":"Counter"}
 * {"v":1,"target":"CoinCounter","cmd":"get","name":"Counter"}
 * {"v":1,"target":"CoinCounter","cmd":"stop","name":"Counter"}
 *
 * Legacy text commands are also supported:
 * - counter_reset, counter_get, counter_dispense_<n>, counter_stop
 */
class CoinCounter {
public:
    // Constructor with relay hopper and relay number
    CoinCounter(uint8_t counterPin, String name, RelayHopper* relayController, uint8_t relayNum);
    
    // Legacy constructor (for backward compatibility)
    CoinCounter(uint8_t counterPin, String name = "Counter");
    
    void begin();
    void update();
    void reset();
    int getCount();
    String getName();
    bool hasNewCoin();
    
    // New method to process commands (instead of handleSerial)
    bool processCommand(const String& cmd, JsonDocument& doc);
    
    // Dispense function
    void dispense(int numberOfCoins);
    bool hasReachedTarget();
    bool hasTimedOut();
    bool isDispensing();
    void stopDispensing();
    
    // Status reporting methods
    bool hasStatusUpdate();
    void clearStatusUpdate();
    bool hasEvent();
    void clearEvent();
    
    // State getters for status reporting
    int getCurrentCount() const { return m_nCurrentCount; }
    int getTargetCount() const { return m_nTargetCount; }
    bool getDispensing() const { return m_bDispensing; }
    bool getTargetReached() const { return m_bTargetReached; }
    bool getTimedOut() const { return m_bTimedOut; }

private:
    // Hardware
    ezButton m_counterButton;           // Button object for coin sensor input
    
    // Basic properties
    String m_strCounterName;            // Name of this counter instance
    int m_nCurrentCount;                // Current count of coins detected
    int m_nTargetCount;                 // Target number of coins to dispense
    
    // State flags (booleans)
    bool m_bNewCoinDetected;            // Flag: new coin was detected
    bool m_bDispensing;                 // Flag: currently dispensing coins
    bool m_bTargetReached;              // Flag: dispensing target count reached
    bool m_bTimedOut;                   // Flag: dispensing operation timed out
    
    // Relay control
    RelayHopper* m_pRelayController;    // Pointer to relay controller
    uint8_t m_nRelayNumber;             // Relay number for this counter (1-3)
    bool m_bHasRelay;                   // Flag: has relay controller attached
    
    // Timing variables (milliseconds)
    unsigned long m_ulLastCoinTime;     // Time when last coin was detected
    unsigned long m_ulLastStatusTime;   // Time when last status was sent
    
    // Status reporting flags
    bool m_bHasStatusUpdate;            // Flag: has new status to report
    bool m_bHasEvent;                   // Flag: has new event to report
    
    // Constants (milliseconds)
    const unsigned long k_ulDebounceDelay = 10;      // Button debounce time
    const unsigned long k_ulTimeoutDuration = 3000;  // Timeout after no coins (3s)
    const unsigned long k_ulStatusInterval = 1000;   // Status update interval (1s)
    
    void checkTimeout();
    
    // Command processing helpers
    void processLegacyCommand(const String& prefix);
    bool processJsonCommand(JsonDocument& doc);
};

#endif // COINCOUNTER_H