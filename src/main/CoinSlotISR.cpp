#include "CoinSlotISR.h"

/**
 * Response JSON Formats (Arduino → Raspberry Pi):
 *
 * Event response (when coin is d// ---- Static volatile (ISR-shared) ====
volatile unsigned long CoinSlotISR::s_ulPulseCount   = 0;
volatile unsigned long CoinSlotISR::s_ulLastPulseTime= 0;
volatile bool         CoinSlotISR::s_bCoinActive     = false;
volatile int          CoinSlotISR::s_nTotalValue     = 0;

CoinSlotISR::CoinSlotISR(uint8_t pulsePin)
: m_u8PulsePin(pulsePin), m_bIsAttached(false), 
  m_bHasStatusUpdate(false), m_bHasEvent(false), m_nLastCoinValue(0) {}:
 * {
 *   "v": 1,
 *   "source": "CoinSlot",
 *   "type": "event",
 *   "ts": 12345678,
 *   "data": {
 *     "coinValue": 10,         // Value of the current coin
 *     "totalValue": 50         // Accumulated value of all coins
 *   }
 * }
 *
 * Status response (when queried with "get" or "status"):
 * {
 *   "v": 1,
 *   "source": "CoinSlot",
 *   "type": "status",
 *   "ts": 12345678,
 *   "data": {
 *     "totalValue": 50,        // Accumulated value of all coins
 *     "attached": true         // Whether the interrupt is attached
 *   }
 * }
 *
 * Acknowledgement response:
 * {
 *   "v": 1,
 *   "source": "CoinSlot",
 *   "type": "ack",
 *   "ts": 12345678,
 *   "data": {
 *     "action": "reset",       // Command that was acknowledged
 *     "ok": true,              // Whether the command succeeded
 *     "status": "detached"     // Optional status message (or error if ok=false)
 *   }
 * }
 * 
 * Command formats (Raspberry Pi → Arduino):
 * 
 * JSON format:
 * {
 *   "v": 1,                    // Protocol version
 *   "target": "CoinSlot",      // Module target
 *   "cmd": "attach"            // Command: "get", "reset", "attach", "detach", "status"
 * }
 * 
 * Legacy text format:
 * get                          // Get current status
 * reset                        // Reset counters
 * attach                       // Attach interrupt (enable coin detection)
 * detach                       // Detach interrupt (disable coin detection)
 * coinslot_start               // Alias for attach
 * coinslot_stop                // Alias for detach
 * status                       // Get full status
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
 * # Command examples
 * 
 * # Get current status
 * status_cmd = {
 *     "v": 1,
 *     "target": "CoinSlot",
 *     "cmd": "status"
 * }
 * 
 * # Enable coin detection
 * enable_cmd = {
 *     "v": 1,
 *     "target": "CoinSlot",
 *     "cmd": "attach"
 * }
 * 
 * # Disable coin detection
 * disable_cmd = {
 *     "v": 1,
 *     "target": "CoinSlot",
 *     "cmd": "detach"
 * }
 * 
 * # Send command
 * ser.write((json.dumps(status_cmd) + '\n').encode())
 * 
 * # Read response
 * response = ser.readline().decode('utf-8').strip()
 * if response:
 *     data = json.loads(response)
 *     if data['type'] == 'status':
 *         total_value = data['data']['totalValue']
 *         is_attached = data['data']['attached']
 *         print(f"Current total: {total_value}, Coin detection: {'ON' if is_attached else 'OFF'}")
 *     
 * # Listen for coin events
 * while True:
 *     response = ser.readline().decode('utf-8').strip()
 *     if not response:
 *         continue
 *         
 *     data = json.loads(response)
 *     if data['type'] == 'event':
 *         coin_value = data['data']['coinValue']
 *         total_value = data['data']['totalValue']
 *         print(f"New coin: {coin_value}, Total: {total_value}")
 */

// ==== Static volatile (ISR-shared) ====
volatile unsigned long CoinSlotISR::s_ulPulseCount   = 0;
volatile unsigned long CoinSlotISR::s_ulLastPulseTime= 0;
volatile bool         CoinSlotISR::s_bCoinActive     = false;
volatile int          CoinSlotISR::s_nTotalValue     = 0;

CoinSlotISR::CoinSlotISR(uint8_t pulsePin)
: m_u8PulsePin(pulsePin), m_bIsAttached(false) {}

// ---- Setup ----
void CoinSlotISR::begin() {
    pinMode(m_u8PulsePin, INPUT_PULLUP);
    m_bIsAttached = false;
}

// ---- Detach interrupt ----
void CoinSlotISR::detach() {
    if (m_bIsAttached) {
        detachInterrupt(digitalPinToInterrupt(m_u8PulsePin));
        m_bIsAttached = false;
        m_bHasStatusUpdate = true; // Signal status change
    }
}

// ---- Reattach interrupt ----
bool CoinSlotISR::reattach() {
    if (!m_bIsAttached) {
        attachInterrupt(digitalPinToInterrupt(m_u8PulsePin), CoinSlotISR::coinPulseISR, FALLING);
        m_bIsAttached = true;
        m_bHasStatusUpdate = true; // Signal status change
        return true;
    }
    return false;  // Already attached
}

// ---- Main loop tick ----
void CoinSlotISR::update() {
    // When pulses have stopped for k_ulCoinDetectTimeout, finalize as one coin
    if (s_bCoinActive && (millis() - s_ulLastPulseTime > k_ulCoinDetectTimeout)) {
        finalizeCoin((int)s_ulPulseCount);
        s_ulPulseCount = 0;
        s_bCoinActive = false;
    }
}

// ---- Legacy Serial handler (to be removed after refactoring) ----
void CoinSlotISR::handleSerial(const String& line) {
    // Forward to new processCommand method for compatibility during refactoring
    StaticJsonDocument<192> doc;
    processCommand(line, doc);
}

// ---- New command processor ----
bool CoinSlotISR::processCommand(const String& cmd, JsonDocument& doc) {
    String trimmedCmd = cmd;
    trimmedCmd.trim();
    if (trimmedCmd.length() == 0) return false;

    bool handled = false;
    
    // Try JSON
    DeserializationError err = deserializeJson(doc, trimmedCmd);
    if (!err && doc.containsKey("v") && doc.containsKey("target") && doc.containsKey("cmd")) {
        if (doc["v"] == 1 && strcmp(doc["target"], "CoinSlot") == 0) {
            handled = processJsonCommand(doc);
        }
    } else {
        // Fallback: legacy text command ("get", "reset")
        processLegacyCommand(trimmedCmd);
        handled = true; // Assume handled for legacy commands
    }
    
    return handled;
}

// ---- ISR ----
void CoinSlotISR::coinPulseISR() {
    unsigned long now = millis();
    if (now - s_ulLastPulseTime > k_ulCoinDebounceTime) {
        s_ulPulseCount++;
        s_ulLastPulseTime = now;
        s_bCoinActive = true;
    }
}

// ---- Coin classification map (unchanged) ----
int CoinSlotISR::getCoinValue(int count) {
    // Original mapping: 1→1, 3→5, 6→10, 9→20; else 0
    switch (count) {
        case 1: return 1;
        case 3: return 5;
        case 6: return 10;
        case 9: return 20;
        default: return 0;
    }
}

// ---- Finalize a coin after pulse window ----
void CoinSlotISR::finalizeCoin(int count) {
    int value = getCoinValue(count);
    s_nTotalValue += value;
    if(value > 0) {
      // Store the last coin value and set event flag instead of sending directly
      ((CoinSlotISR*)nullptr)->m_nLastCoinValue = value; // Access non-static from static
      ((CoinSlotISR*)nullptr)->m_bHasEvent = true;       // Set flag for main loop to handle
    }
}

// ---- JSON command processor ----
bool CoinSlotISR::processJsonCommand(JsonDocument& doc) {
    const char* cmd = doc["cmd"];
    if (!cmd) {
        return false;
    }

    bool handled = true;

    if (strcmp(cmd, "get") == 0 || strcmp(cmd, "status") == 0) {
        m_bHasStatusUpdate = true;
    } else if (strcmp(cmd, "reset") == 0) {
        s_nTotalValue = 0;
        m_bHasStatusUpdate = true;
    } else if (strcmp(cmd, "attach") == 0) {
        bool result = reattach();
        // Status update flag is set in reattach()
    } else if (strcmp(cmd, "detach") == 0) {
        detach();
        // Status update flag is set in detach()
    } else {
        handled = false;
    }
    
    return handled;
}

// ---- Legacy command processor (text) ----
void CoinSlotISR::processLegacyCommand(const String& cmd) {
    if (cmd == "get" || cmd == "status") {
        m_bHasStatusUpdate = true;
    } else if (cmd == "reset") {
        s_nTotalValue = 0;
        m_bHasStatusUpdate = true;
    } else if (cmd == "attach" || cmd == "coinslot_start") {
        reattach();
        // Status update flag is set in reattach()
    } else if (cmd == "detach" || cmd == "coinslot_stop") {
        detach();
        // Status update flag is set in detach()
    }
}

// These methods have been removed in favor of centralized serial handling
// The main.ino file will now handle all serial communication using SerialComm
