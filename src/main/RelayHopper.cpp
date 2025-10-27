#include "RelayHopper.h"

/**
 * Response JSON Formats (Arduino → Raspberry Pi):
 *
 * Status response:
 * {
 *   "v": 1,
 *   "source": "Relay",
 *   "type": "status",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "Relay",
 *     "states": ["ON", "OFF", "OFF"]  // States of relay 1, 2, and 3
 *   }
 * }
 *
 * Acknowledgement response (after successful command):
 * {
 *   "v": 1,
 *   "source": "Relay",
 *   "type": "ack",
 *   "ts": 12345678,
 *   "data": {
 *     "relay": 1,            // Relay number affected
 *     "action": "set",       // Action performed
 *     "state": "ON",         // New state of the relay
 *     "ok": true             // Always true for acknowledgements
 *   }
 * }
 *
 * Error response (separate from acknowledgements):
 * {
 *   "v": 1,
 *   "source": "Relay",
 *   "type": "error",
 *   "ts": 12345678,
 *   "data": {
 *     "action": "setRelay",            // Action that failed
 *     "relay": 1,                      // Optional: relay number if applicable
 *     "error": "missing_parameters"    // Error message
 *   }
 * }
 */
RelayHopper::RelayHopper(uint8_t pin1, uint8_t pin2, uint8_t pin3, String name)
    : m_strRelayName(name), m_ulLastStatusTime(0), m_bHasStatusUpdate(false),
      m_nLastRelayChanged(0), m_bLastCommandSucceeded(false), m_pLastErrorMessage(nullptr) {
    m_nRelayPins[0] = pin1;
    m_nRelayPins[1] = pin2;
    m_nRelayPins[2] = pin3;
    
    // Initialize all relays to OFF state
    for (int i = 0; i < 3; i++) {
        m_bRelayStates[i] = true; // true = OFF (HIGH), false = ON (LOW)
    }
}

void RelayHopper::begin() {
    for (int i = 0; i < 3; i++) {
        pinMode(m_nRelayPins[i], OUTPUT);
        digitalWrite(m_nRelayPins[i], HIGH); // Start with relays OFF
    }
    
    // Signal that status should be sent
    m_bHasStatusUpdate = true;
}

void RelayHopper::update() {
    // No continuous state changes to monitor for relays,
    // but we could send periodic status updates if needed
    // This is a place to add any periodic status updates or health checks
    
    // Example usage from Raspberry Pi (Python):
    /*
    # Python code example for Raspberry Pi
    import serial
    import json
    import time
    
    ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
    time.sleep(2)  # Allow Arduino to reset
    
    # Turn ON relay 1
    command = {
        "v": 1,
        "target": "Relay",
        "cmd": "setRelay",
        "value": 1,
        "state": "on"
    }
    
    # Send command
    ser.write((json.dumps(command) + '\n').encode())
    
    # Read response (parse as JSON)
    response = ser.readline().decode('utf-8').strip()
    if response:
        resp_json = json.loads(response)
        print(f"Response: {resp_json}")
        
        # Check for success
        if resp_json['type'] == 'ack' and resp_json['data'].get('ok') == True:
            print("Command confirmed!")
        elif resp_json['type'] == 'error':
            print(f"Error: {resp_json['data'].get('error')}")
    */
}

void RelayHopper::setRelay(uint8_t relayNum, bool state) {
    if (relayNum >= 1 && relayNum <= 3) {
        // Update pin state (HIGH = OFF, LOW = ON in typical relay modules)
        digitalWrite(m_nRelayPins[relayNum - 1], state ? HIGH : LOW);
        
        // Update internal state tracking
        m_bRelayStates[relayNum - 1] = state;
        
        // Update last relay changed
        m_nLastRelayChanged = relayNum;
        m_bLastCommandSucceeded = true;
        
        // Signal that status should be updated
        m_bHasStatusUpdate = true;
    }
}

bool RelayHopper::getRelayState(uint8_t relayNum) {
    if (relayNum >= 1 && relayNum <= 3) {
        return m_bRelayStates[relayNum - 1];
    }
    return false;
}

void RelayHopper::handleSerial(const String& line) {
    // Forward to new processCommand method for compatibility during refactoring
    StaticJsonDocument<256> doc;
    processCommand(line, doc);
}

bool RelayHopper::processCommand(const String& cmd, JsonDocument& doc) {
    // Try to parse as JSON
    DeserializationError error = deserializeJson(doc, cmd);
    bool handled = false;
    
    if (!error) {
        // Check if this is a valid JSON command for RelayHopper
        if (doc.containsKey("v") && doc.containsKey("target") && doc.containsKey("cmd")) {
            
            // Check if it's for RelayHopper and it's the right version
            if (doc["v"] == 1 && strcmp(doc["target"], "Relay") == 0) {
                handled = processJsonCommand(doc);
            }
        }
    }
    
    if (!handled) {
        // Fall back to the original command format if not JSON or not for us
        processLegacyCommand(cmd);
        handled = true; // Assume legacy commands are always handled
    }
    
    return handled;
}

// These methods have been removed in favor of centralized serial handling
// The main.ino file will now handle all serial communication using SerialComm

bool RelayHopper::processJsonCommand(JsonDocument& doc) {
    const char* cmdName = doc["cmd"];
    bool handled = true;
    
    if (strcmp(cmdName, "setRelay") == 0) {
        // Check for required parameters
        if (!doc.containsKey("value") || !doc.containsKey("state")) {
            m_bLastCommandSucceeded = false;
            m_pLastErrorMessage = "missing_parameters";
            return false;
        }
        
        int relayNumber = doc["value"];
        const char* stateStr = doc["state"];
        bool state;
        
        // Convert string state to bool (true = OFF, false = ON for typical relay modules)
        if (strcmp(stateStr, "on") == 0) {
            state = false; // ON = LOW = false
        } else if (strcmp(stateStr, "off") == 0) {
            state = true; // OFF = HIGH = true
        } else {
            m_bLastCommandSucceeded = false;
            m_pLastErrorMessage = "invalid_state";
            m_nLastRelayChanged = relayNumber;
            return false;
        }
        
        // Set the relay
        digitalWrite(m_nRelayPins[relayNumber - 1], state ? HIGH : LOW);
        m_bRelayStates[relayNumber - 1] = state;
        
        // Update status for main loop
        m_nLastRelayChanged = relayNumber;
        m_bLastCommandSucceeded = true;
        m_bHasStatusUpdate = true;
    } 
    else if (strcmp(cmdName, "get") == 0) {
        // Signal status update
        m_bHasStatusUpdate = true;
    }
    else {
        // Unknown command
        m_bLastCommandSucceeded = false;
        m_pLastErrorMessage = "unknown_command";
        handled = false;
    }
    
    return handled;
}

void RelayHopper::processLegacyCommand(const String& cmd) {
    if (cmd == "relay1_on") setRelay(1, false);
    else if (cmd == "relay1_off") setRelay(1, true);
    else if (cmd == "relay2_on") setRelay(2, false);
    else if (cmd == "relay2_off") setRelay(2, true);
    else if (cmd == "relay3_on") setRelay(3, false);
    else if (cmd == "relay3_off") setRelay(3, true);
}