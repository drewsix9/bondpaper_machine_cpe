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
    : m_strRelayName(name), m_ulLastStatusTime(0) {
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
    
    // Send initial status
    sendStatusJson();
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
        
        // For legacy command support, send standard acknowledgement
        sendAckJson("set", relayNum, state ? "OFF" : "ON");
    }
}

bool RelayHopper::getRelayState(uint8_t relayNum) {
    if (relayNum >= 1 && relayNum <= 3) {
        return m_bRelayStates[relayNum - 1];
    }
    return false;
}

void RelayHopper::handleSerial(const String& line) {
    // Try to parse as JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, line);
    
    if (!error) {
        // Check if this is a valid JSON command for RelayHopper
        if (doc.containsKey("v") && doc.containsKey("target") && doc.containsKey("cmd")) {
            
            // Check if it's for RelayHopper and it's the right version
            if (doc["v"] == 1 && strcmp(doc["target"], "Relay") == 0) {
                processJsonCommand(doc);
                return;
            }
        }
    }
    
    // Fall back to the original command format if not JSON or not for us
    processLegacyCommand(line);
}

void RelayHopper::sendStatusJson() {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "Relay";
    doc["type"] = "status";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strRelayName;
    
    // Create array for relay states
    JsonArray states = data.createNestedArray("states");
    for (int i = 0; i < 3; i++) {
        states.add(m_bRelayStates[i] ? "OFF" : "ON");
    }
    
    serializeJson(doc, Serial);
    Serial.println();
}

void RelayHopper::sendAckJson(const char* action, uint8_t relay, const char* state) {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "Relay";
    doc["type"] = "ack";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["relay"] = relay;
    data["action"] = action;
    data["state"] = state;
    data["ok"] = true;
    
    serializeJson(doc, Serial);
    Serial.println();
}

void RelayHopper::sendErrorJson(const char* action, uint8_t relay, const char* errorMessage) {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "Relay";
    doc["type"] = "error";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["action"] = action;
    
    if (relay > 0) {
        data["relay"] = relay;
    }
    
    data["error"] = errorMessage;
    
    serializeJson(doc, Serial);
    Serial.println();
}

void RelayHopper::processJsonCommand(JsonDocument& doc) {
    const char* cmdName = doc["cmd"];
    
    if (strcmp(cmdName, "setRelay") == 0) {
        // Check for required parameters
        if (!doc.containsKey("value") || !doc.containsKey("state")) {
            sendErrorJson("setRelay", 0, "missing_parameters");
            return;
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
            sendErrorJson("setRelay", relayNumber, "invalid_state");
            return;
        }
        
        // Set the relay
        digitalWrite(m_nRelayPins[relayNumber - 1], state ? HIGH : LOW);
        m_bRelayStates[relayNumber - 1] = state;
        
        // Send acknowledgement
        sendAckJson("set", relayNumber, state ? "OFF" : "ON");
    } 
    else if (strcmp(cmdName, "get") == 0) {
        // Send current status
        sendStatusJson();
    }
    else {
        // Unknown command
        sendErrorJson(cmdName, 0, "unknown_command");
    }
}

void RelayHopper::processLegacyCommand(const String& cmd) {
    if (cmd == "relay1_on") setRelay(1, false);
    else if (cmd == "relay1_off") setRelay(1, true);
    else if (cmd == "relay2_on") setRelay(2, false);
    else if (cmd == "relay2_off") setRelay(2, true);
    else if (cmd == "relay3_on") setRelay(3, false);
    else if (cmd == "relay3_off") setRelay(3, true);
}