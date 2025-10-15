#ifndef RELAYHOPPER_H
#define RELAYHOPPER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SerialComm.h"

/**
 * RelayHopper - Controls relay modules for coin hoppers
 * 
 * JSON Command Structure (Raspberry Pi → Arduino):
 * {
 *   "v": 1,                     // Protocol version
 *   "target": "Relay",          // Must be "Relay" for this module
 *   "cmd": "setRelay",          // Command: "setRelay" or "get"
 *   "value": 1,                 // For setRelay: relay number (1-3)
 *   "state": "on",              // For setRelay: "on" or "off"
 * }
 * 
 * Example commands:
 * {"v":1,"target":"Relay","cmd":"setRelay","value":1,"state":"on"}
 * {"v":1,"target":"Relay","cmd":"setRelay","value":2,"state":"off"}
 * {"v":1,"target":"Relay","cmd":"get"}
 *
 * Legacy text commands are also supported:
 * - relay1_on, relay1_off, relay2_on, relay2_off, relay3_on, relay3_off
 */
class RelayHopper {
public:
    RelayHopper(uint8_t pin1, uint8_t pin2, uint8_t pin3, String name = "Relay");
    void begin();
    void update();
    void setRelay(uint8_t relayNum, bool state);
    
    // Legacy method - will be removed after refactoring
    void handleSerial(const String& line);
    
    // New method to process commands
    bool processCommand(const String& cmd, JsonDocument& doc);
    
    bool getRelayState(uint8_t relayNum);
    String getName() const { return m_strRelayName; }
    
    // Status reporting methods
    bool hasStatusUpdate() const { return m_bHasStatusUpdate; }
    void clearStatusUpdate() { m_bHasStatusUpdate = false; }
    
    // Get current relay states
    bool* getRelayStates() { return m_bRelayStates; }

private:
    // Hardware
    uint8_t m_nRelayPins[3];
    
    // Basic properties
    String m_strRelayName;            // Name of this relay instance
    bool m_bRelayStates[3];           // Current state of each relay (true = OFF, false = ON)
    
    // Status reporting flags
    bool m_bHasStatusUpdate;          // Flag: has new status to report
    
    // Timing variables (milliseconds)
    unsigned long m_ulLastStatusTime; // Time when last status was sent
    
    // Constants (milliseconds)
    const unsigned long k_ulStatusInterval = 1000;   // Status update interval (1s)
    
    // Command handling
    bool processJsonCommand(JsonDocument& doc);
    void processLegacyCommand(const String& cmd);
    
    // Last command result tracking
    uint8_t m_nLastRelayChanged;
    bool m_bLastCommandSucceeded;
    const char* m_pLastErrorMessage;
};

#endif // RELAYHOPPER_H
