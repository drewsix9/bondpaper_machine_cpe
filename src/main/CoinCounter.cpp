#include "CoinCounter.h"

/**
 * Response JSON Formats (Arduino → Raspberry Pi):
 *
 * Status response:
 * {
 *   "v": 1,
 *   "source": "CoinCounter",
 *   "type": "status",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "Counter",
 *     "count": 3,
 *     "target": 10,            // Only included when dispensing
 *     "status": "dispensing"   // "dispensing" or "idle"
 *   }
 * }
 *
 * Event response (when target reached):
 * {
 *   "v": 1,
 *   "source": "CoinCounter",
 *   "type": "event",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "Counter",
 *     "event": "target_reached",
 *     "count": 10,
 *     "target": 10
 *   }
 * }
 *
 * Error response (timeout):
 * {
 *   "v": 1,
 *   "source": "CoinCounter",
 *   "type": "error",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "Counter",
 *     "event": "timeout",
 *     "final": 3,
 *     "target": 10
 *   }
 * }
 *
 * Acknowledgement response:
 * {
 *   "v": 1,
 *   "source": "CoinCounter",
 *   "type": "ack",
 *   "data": {
 *     "name": "Counter",
 *     "action": "dispense",
 *     "ok": true,
 *     "value": 10,           // For dispense command
 *     "status": "started"    // Additional context when relevant
 *   }
 * }
 *
 * Example Python code for Raspberry Pi:
 * 
 * import serial
 * import json
 * import time
 * 
 * ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
 * time.sleep(2)  # Allow Arduino to reset
 * 
 * # Command to dispense 10 coins
 * command = {
 *     "v": 1,
 *     "target": "CoinCounter",
 *     "cmd": "dispense",
 *     "name": "Counter",
 *     "value": 10
 * }
 * 
 * # Send command as single line JSON (no pretty printing)
 * ser.write((json.dumps(command) + '\n').encode())
 * 
 * # Read and process responses until operation completes
 * while True:
 *     response = ser.readline().decode('utf-8').strip()
 *     if not response:
 *         continue
 *         
 *     data = json.loads(response)
 *     print(f"Received: {data['type']}")
 *     
 *     # Handle different response types
 *     if data['type'] == 'event' and data['data'].get('event') == 'target_reached':
 *         print("Dispensing complete!")
 *         break
 *     elif data['type'] == 'error':
 *         print(f"Error: {data['data'].get('event')}")
 *         break
 */
CoinCounter::CoinCounter(uint8_t counterPin, String name, RelayHopper* relayController, uint8_t relayNum)
    : m_counterButton(counterPin), m_strCounterName(name), m_nCurrentCount(0), 
      m_nTargetCount(0), m_bNewCoinDetected(false), m_bDispensing(false),
      m_bTargetReached(false), m_bTimedOut(false), m_ulLastCoinTime(0), m_ulLastStatusTime(0),
      m_pRelayController(relayController), m_nRelayNumber(relayNum), m_bHasRelay(true) {}
      
// Legacy constructor (no relay control)
CoinCounter::CoinCounter(uint8_t counterPin, String name)
    : m_counterButton(counterPin), m_strCounterName(name), m_nCurrentCount(0), 
      m_nTargetCount(0), m_bNewCoinDetected(false), m_bDispensing(false),
      m_bTargetReached(false), m_bTimedOut(false), m_ulLastCoinTime(0), m_ulLastStatusTime(0),
      m_pRelayController(nullptr), m_nRelayNumber(0), m_bHasRelay(false) {}

void CoinCounter::begin() {
    m_counterButton.setDebounceTime(k_ulDebounceDelay);
}

void CoinCounter::update() {
    m_counterButton.loop();
    
    if (m_counterButton.isPressed()) {
        m_nCurrentCount++;
        m_bNewCoinDetected = true;
        m_ulLastCoinTime = millis();
        m_bTimedOut = false;
        
        // Check if target is reached
        if (m_bDispensing && m_nTargetCount > 0 && m_nCurrentCount >= m_nTargetCount) {
            m_bTargetReached = true;
            m_bDispensing = false;
            
            // Turn off the relay if we have one
            if (m_bHasRelay && m_pRelayController != nullptr) {
                m_pRelayController->setRelay(m_nRelayNumber, true); // true = OFF for typical relay modules
            }
            
            // Send the target reached event - this is a notification, not a command response
            sendEventJson("event", "target_reached");
        }
    }
    
    // Send periodic status updates while dispensing
    if (m_bDispensing && (millis() - m_ulLastStatusTime >= k_ulStatusInterval)) {
        sendStatusJson();
        m_ulLastStatusTime = millis();
    }
    
    if (m_bDispensing) {
        checkTimeout();
    }
}

void CoinCounter::checkTimeout() {
    if (m_bDispensing && !m_bTargetReached && (millis() - m_ulLastCoinTime) >= k_ulTimeoutDuration) {
        m_bTimedOut = true;
        m_bDispensing = false;
        
        // Turn off the relay if we have one
        if (m_bHasRelay && m_pRelayController != nullptr) {
            m_pRelayController->setRelay(m_nRelayNumber, true); // true = OFF for typical relay modules
        }
        
        // Keep sending timeout notifications - this is an event, not a command response
        sendTimeoutJson();
    }
}

void CoinCounter::reset() {
    m_nCurrentCount = 0;
    m_bNewCoinDetected = false;
    m_bTargetReached = false;
    m_bTimedOut = false;
    m_bDispensing = false;
    m_nTargetCount = 0;
    m_ulLastCoinTime = 0;
    m_ulLastStatusTime = 0;
    // Event will be sent by the command handler if needed
}

void CoinCounter::dispense(int numberOfCoins) {
    if (numberOfCoins <= 0) {
        // Invalid amount, don't start dispensing
        return;
    }
    
    m_nTargetCount = numberOfCoins;
    m_nCurrentCount = 0;
    m_bDispensing = true;
    m_bTargetReached = false;
    m_bTimedOut = false;
    m_ulLastCoinTime = millis();
    m_ulLastStatusTime = millis();
    
    // If we have a relay controller, turn ON the relay for this counter
    if (m_bHasRelay && m_pRelayController != nullptr) {
        m_pRelayController->setRelay(m_nRelayNumber, false); // false = ON for typical relay modules
    }
    
    // Send initial status after starting
    sendStatusJson();
}

bool CoinCounter::hasReachedTarget() {
    return m_bTargetReached;
}

bool CoinCounter::hasTimedOut() {
    return m_bTimedOut;
}

bool CoinCounter::isDispensing() {
    return m_bDispensing;
}

void CoinCounter::stopDispensing() {
    m_bDispensing = false;
    
    // If we have a relay controller, turn OFF the relay for this counter
    if (m_bHasRelay && m_pRelayController != nullptr) {
        m_pRelayController->setRelay(m_nRelayNumber, true); // true = OFF for typical relay modules
    }
    
    // Event will be sent by the command handler if needed
}

int CoinCounter::getCount() {
    return m_nCurrentCount;
}

String CoinCounter::getName() {
    return m_strCounterName;
}

bool CoinCounter::hasNewCoin() {
    if (m_bNewCoinDetected) {
        m_bNewCoinDetected = false;
        return true;
    }
    return false;
}

void CoinCounter::handleSerial(const String& cmd) {
    // Try to parse as JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, cmd);
    
    if (!error) {
        // Check if this is a valid JSON command for CoinCounter
        if (doc.containsKey("v") && doc.containsKey("target") && 
            doc.containsKey("cmd") && doc.containsKey("name")) {
            
            // Check if it's for CoinCounter and it's the right version
            if (doc["v"] == 1 && strcmp(doc["target"], "CoinCounter") == 0) {
                processJsonCommand(doc);
                return;
            }
        }
    }
    
    // Fall back to the original command format if not JSON or not for us
    processLegacyCommand(cmd);
}

void CoinCounter::sendStatusJson() {
    StaticJsonDocument<200> doc;
    
    doc["v"] = 1;
    doc["source"] = "CoinCounter";
    doc["type"] = "status";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strCounterName;
    data["count"] = m_nCurrentCount;
    
    if (m_bDispensing && m_nTargetCount > 0) {
        data["target"] = m_nTargetCount;
        data["status"] = "dispensing";
    } else {
        data["status"] = "idle";
    }
    
    serializeJson(doc, Serial);
    Serial.println();
}

void CoinCounter::sendEventJson(const char* eventType, const char* event) {
    StaticJsonDocument<200> doc;
    
    doc["v"] = 1;
    doc["source"] = "CoinCounter";
    doc["type"] = eventType;
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strCounterName;
    data["event"] = event;
    data["count"] = m_nCurrentCount;
    
    if (m_nTargetCount > 0) {
        data["target"] = m_nTargetCount;
    }
    
    serializeJson(doc, Serial);
    Serial.println();
}

void CoinCounter::sendTimeoutJson() {
    StaticJsonDocument<200> doc;
    
    doc["v"] = 1;
    doc["source"] = "CoinCounter";
    doc["type"] = "error";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strCounterName;
    data["event"] = "timeout";
    data["final"] = m_nCurrentCount;
    data["target"] = m_nTargetCount;
    
    serializeJson(doc, Serial);
    Serial.println();
}

void CoinCounter::processLegacyCommand(const String& cmd) {
    // Get lowercase version of counter name for comparison
    String prefix = m_strCounterName;
    prefix.toLowerCase();
    
    // Get lowercase version of command for comparison
    String lowerCmd = cmd;
    lowerCmd.toLowerCase();

    if (lowerCmd == prefix + "_reset") {
        reset();
        sendAckJson("reset");
    } else if (lowerCmd == prefix + "_get") {
        sendStatusJson();
    } else if (lowerCmd.startsWith(prefix + "_dispense_")) {
        int amount = lowerCmd.substring((prefix + "_dispense_").length()).toInt();
        dispense(amount);
        sendAckJson("dispense", true, amount, "started");
    } else if (lowerCmd == prefix + "_stop") {
        stopDispensing();
        sendAckJson("stop");
    }
}

void CoinCounter::processJsonCommand(JsonDocument& doc) {
    const char* cmdName = doc["cmd"];
    const char* targetName = doc["name"];
    
    // Only process commands for this counter
    if (strcmp(targetName, m_strCounterName.c_str()) != 0) {
        return;
    }
    
    if (strcmp(cmdName, "reset") == 0) {
        reset();
        sendAckJson("reset");
    } 
    else if (strcmp(cmdName, "get") == 0) {
        sendStatusJson();
    } 
    else if (strcmp(cmdName, "dispense") == 0) {
        if (doc.containsKey("value")) {
            int amount = doc["value"];
            dispense(amount);
            sendAckJson("dispense", true, amount, "started");
        } else {
            // Missing value parameter
            sendAckJson("dispense", false, 0, "missing_value");
        }
    } 
    else if (strcmp(cmdName, "stop") == 0) {
        stopDispensing();
        sendAckJson("stop");
    }
}

void CoinCounter::sendAckJson(const char* action, bool ok, int value, const char* status) {
    StaticJsonDocument<200> doc;
    
    doc["v"] = 1;
    doc["source"] = "CoinCounter";
    doc["type"] = "ack";
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strCounterName;
    data["action"] = action;
    data["ok"] = ok;
    
    if (value > 0) {
        data["value"] = value;
    }
    
    if (status != nullptr) {
        data["status"] = status;
    }
    
    serializeJson(doc, Serial);
    Serial.println();
}