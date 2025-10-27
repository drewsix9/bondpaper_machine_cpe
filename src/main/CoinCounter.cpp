#include "CoinCounter.h"
#include "SerialComm.h"

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
      m_pRelayController(relayController), m_nRelayNumber(relayNum), m_bHasRelay(true),
      m_bHasStatusUpdate(false), m_bHasEvent(false) {}
      
// Legacy constructor (no relay control)
CoinCounter::CoinCounter(uint8_t counterPin, String name)
    : m_counterButton(counterPin), m_strCounterName(name), m_nCurrentCount(0), 
      m_nTargetCount(0), m_bNewCoinDetected(false), m_bDispensing(false),
      m_bTargetReached(false), m_bTimedOut(false), m_ulLastCoinTime(0), m_ulLastStatusTime(0),
      m_pRelayController(nullptr), m_nRelayNumber(0), m_bHasRelay(false),
      m_bHasStatusUpdate(false), m_bHasEvent(false) {}

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
            
            // Set event flag instead of sending directly
            m_bHasEvent = true;
        }
    }
    
    // Update status flag instead of sending directly
    if (m_bDispensing && (millis() - m_ulLastStatusTime >= k_ulStatusInterval)) {
        m_bHasStatusUpdate = true;
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
        
        // Set event flag instead of sending directly
        m_bHasEvent = true;
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
    m_bHasStatusUpdate = true; // Signal that status should be updated
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
    
    // Signal that status should be updated
    m_bHasStatusUpdate = true;
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
    
    // Signal that status should be updated
    m_bHasStatusUpdate = true;
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

// New method to process commands (instead of handleSerial)
bool CoinCounter::processCommand(const String& cmd, JsonDocument& doc) {
    bool handled = false;
    
    // Check if it's a JSON command first
    DeserializationError error = deserializeJson(doc, cmd);
    
    if (!error) {
        // Check if this is a valid JSON command for CoinCounter
        if (doc.containsKey("v") && doc.containsKey("target") && 
            doc.containsKey("cmd") && doc.containsKey("name")) {
            
            // Check if it's for CoinCounter and it's the right version
            if (doc["v"] == 1 && strcmp(doc["target"], "CoinCounter") == 0) {
                const char* targetName = doc["name"];
                
                // Only process commands for this counter
                if (strcmp(targetName, m_strCounterName.c_str()) == 0) {
                    handled = processJsonCommand(doc);
                }
            }
        }
    } else {
        // Try legacy command format
        processLegacyCommand(cmd);
        handled = true; // Assume handled for legacy commands
    }
    
    return handled;
}

// Status reporting methods
bool CoinCounter::hasStatusUpdate() {
    return m_bHasStatusUpdate;
}

void CoinCounter::clearStatusUpdate() {
    m_bHasStatusUpdate = false;
}

bool CoinCounter::hasEvent() {
    return m_bHasEvent;
}

void CoinCounter::clearEvent() {
    m_bHasEvent = false;
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
    } else if (lowerCmd == prefix + "_get") {
        m_bHasStatusUpdate = true;
    } else if (lowerCmd.startsWith(prefix + "_dispense_")) {
        int amount = lowerCmd.substring((prefix + "_dispense_").length()).toInt();
        dispense(amount);
    } else if (lowerCmd == prefix + "_stop") {
        stopDispensing();
    }
}

bool CoinCounter::processJsonCommand(JsonDocument& doc) {
    const char* cmdName = doc["cmd"];
    bool handled = true;
    
    if (strcmp(cmdName, "reset") == 0) {
        reset();
        // Status update flag is set in reset()
    } 
    else if (strcmp(cmdName, "get") == 0) {
        m_bHasStatusUpdate = true;
    } 
    else if (strcmp(cmdName, "dispense") == 0) {
        if (doc.containsKey("value")) {
            int amount = doc["value"];
            dispense(amount);
            // Status update flag is set in dispense()
        } else {
            // Missing value parameter
            handled = false;
        }
    } 
    else if (strcmp(cmdName, "stop") == 0) {
        stopDispensing();
        // Status update flag is set in stopDispensing()
    }
    else {
        handled = false;
    }
    
    return handled;
}