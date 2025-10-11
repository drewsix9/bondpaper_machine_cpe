#include "StepperMotor.h"

/**
 * Response JSON Formats (Arduino → Raspberry Pi):
 *
 * Status response:
 * {
 *   "v": 1,
 *   "source": "PaperDispenser",
 *   "type": "status",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "PaperDispenser1",  // Name of this dispenser instance
 *     "action": "dispense",
 *     "current": 2,
 *     "total": 5,
 *     "status": "in_progress"      // "idle", "homing", "in_progress", "ramping_down", "complete", "error"
 *   }
 * }
 *
 * Event response (when operation completes):
 * {
 *   "v": 1,
 *   "source": "PaperDispenser",
 *   "type": "event",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "PaperDispenser1",  // Name of this dispenser instance
 *     "event": "dispense_complete",
 *     "total": 5
 *   }
 * }
 *
 * Error response:
 * {
 *   "v": 1,
 *   "source": "PaperDispenser",
 *   "type": "error",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "PaperDispenser1",  // Name of this dispenser instance
 *     "error": "jammed",
 *     "details": "Limit switch not triggered after timeout",
 *     "current": 2,
 *     "total": 5
 *   }
 * }
 *
 * Acknowledgement response:
 * {
 *   "v": 1,
 *   "source": "PaperDispenser",
 *   "type": "ack",
 *   "ts": 12345678,
 *   "data": {
 *     "name": "PaperDispenser1",  // Name of this dispenser instance
 *     "action": "dispense",
 *     "ok": true,
 *     "value": 5,             // Number of papers or steps depending on action
 *     "status": "started"     // Additional context when relevant
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
 * # Command to dispense 5 papers
 * command = {
 *     "v": 1,
 *     "target": "PaperDispenser",
 *     "cmd": "dispense",
 *     "name": "PaperDispenser1",
 *     "value": 5
 * }
 * 
 * # Send command
 * ser.write((json.dumps(command) + '\n').encode())
 * 
 * # Read responses until operation completes
 * while True:
 *     response = ser.readline().decode('utf-8').strip()
 *     if not response:
 *         continue
 *         
 *     data = json.loads(response)
 *     print(f"Received: {data['type']}")
 *     
 *     if data['type'] == 'event' and data['data'].get('event') == 'dispense_complete':
 *         print("Dispensing complete!")
 *         break
 *     elif data['type'] == 'error':
 *         print(f"Error: {data['data'].get('error')}")
 *         break
 */

// StepperMotor Class Implementation
StepperMotor::StepperMotor(uint8_t pulsePin, uint8_t dirPin) {
    m_nPulsePin = pulsePin;
    m_nDirPin = dirPin;
    m_ulStepDelay = 1000; // Default 1ms delay
}

void StepperMotor::begin() {
    pinMode(m_nPulsePin, OUTPUT);
    pinMode(m_nDirPin, OUTPUT);
    digitalWrite(m_nPulsePin, LOW);
    digitalWrite(m_nDirPin, LOW);
}

void StepperMotor::rotate(int steps, uint8_t direction, unsigned int delayMicros) {
    // Set direction
    digitalWrite(m_nDirPin, direction == CW ? HIGH : LOW);
    
    // Perform steps
    for (int i = 0; i < abs(steps); i++) {
        step();
        delayMicroseconds(delayMicros);
    }
}

void StepperMotor::setSpeed(unsigned int delayMicros) {
    m_ulStepDelay = delayMicros;
}

void StepperMotor::step() {
    digitalWrite(m_nPulsePin, HIGH);
    delayMicroseconds(m_ulStepDelay);
    digitalWrite(m_nPulsePin, LOW);
    delayMicroseconds(m_ulStepDelay);
}

// PaperDispenser Class Implementation
PaperDispenser::PaperDispenser(uint8_t pulsePin, uint8_t dirPin, uint8_t in1Pin, 
                               uint8_t in2Pin, uint8_t enPin, uint8_t limitSwitchPin, 
                               int stepperSteps, String name) 
    : m_stepper(pulsePin, dirPin), m_strName(name) {
    m_nIn1Pin = in1Pin;
    m_nIn2Pin = in2Pin;
    m_nEnPin = enPin;
    m_nLimitSwitchPin = limitSwitchPin;
    m_nStepperSteps = stepperSteps;
    
    // Initialize state variables
    m_nCurrentPaper = 0;
    m_nTotalPapers = 0;
    m_bDispensing = false;
    m_bRampingDown = false;
    m_bOperationComplete = false;
    m_bStopped = false;
    m_eState = IDLE;
    m_ulLastStatusTime = 0;
    m_ulOperationStartTime = 0;
    m_ulRampDownStartTime = 0;
}

void PaperDispenser::begin() {
    // Initialize stepper motor
    m_stepper.begin();
    
    // Initialize DC motor pins
    pinMode(m_nIn1Pin, OUTPUT);
    pinMode(m_nIn2Pin, OUTPUT);
    pinMode(m_nEnPin, OUTPUT);
    
    // Initialize limit switch
    pinMode(m_nLimitSwitchPin, INPUT_PULLUP);
    
    // Set DC motor to full speed and stop
    analogWrite(m_nEnPin, 255);
    dcMotorStop();
    
    // Set initial state
    m_eState = IDLE;
    
    // Send initial status
    sendStatusJson();
}

void PaperDispenser::update() {
    // Update state machine
    updateStateMachine();
    
    // Send periodic status updates while active
    if (m_eState != IDLE && m_eState != COMPLETE && 
        (millis() - m_ulLastStatusTime >= k_ulStatusInterval)) {
        sendStatusJson();
        m_ulLastStatusTime = millis();
    }
}

void PaperDispenser::updateStateMachine() {
    switch (m_eState) {
        case IDLE:
            // Nothing to do in idle state
            break;
            
        case HOMING:
            // In homing state, check if limit switch is pressed
            if (isLimitSwitchPressed()) {
                dcMotorStop();
                
                // If we're dispensing, move to DISPENSING state
                if (m_bDispensing) {
                    m_eState = DISPENSING;
                } else {
                    m_eState = IDLE;
                }
            }
            break;
            
        case DISPENSING:
            // Check if we've dispensed all papers
            if (m_nCurrentPaper >= m_nTotalPapers || m_bStopped) {
                // Finished dispensing, move to RAMPING_DOWN state
                m_eState = RAMPING_DOWN;
                m_bRampingDown = true;
                m_ulRampDownStartTime = millis();
                dcMotorReverse();
            } else if (!isLimitSwitchPressed()) {
                // If limit switch not pressed, go back to homing
                m_eState = HOMING;
                dcMotorForward();
            } else {
                // Dispense a paper
                m_stepper.rotate(m_nStepperSteps, CW);
                m_nCurrentPaper++;
                
                // Small delay between papers
                delay(100);
                
                // Send updated status
                sendStatusJson();
            }
            break;
            
        case RAMPING_DOWN:
            // Check if ramp down time has elapsed
            if (millis() - m_ulRampDownStartTime >= k_ulRampDownTime) {
                dcMotorStop();
                m_bRampingDown = false;
                
                // Operation complete
                m_eState = COMPLETE;
                m_bOperationComplete = true;
                
                // Send completion event
                sendEventJson("dispense_complete");
            }
            break;
            
        case COMPLETE:
            // Reset state to idle
            m_bDispensing = false;
            m_nCurrentPaper = 0;
            m_nTotalPapers = 0;
            m_eState = IDLE;
            break;
            
        case ERROR:
            // Stay in error state until reset
            break;
    }
}

void PaperDispenser::dispense(int numOfPapers) {
    if (numOfPapers <= 0) {
        // Invalid number of papers
        sendErrorJson("invalid_parameter", "Number of papers must be greater than zero");
        return;
    }
    
    // Reset state
    m_nCurrentPaper = 0;
    m_nTotalPapers = numOfPapers;
    m_bDispensing = true;
    m_bRampingDown = false;
    m_bOperationComplete = false;
    m_bStopped = false;
    
    // Start homing to find initial position
    m_eState = HOMING;
    dcMotorForward();
    
    // Record start time
    m_ulOperationStartTime = millis();
    m_ulLastStatusTime = millis();
    
    // Send initial status
    sendStatusJson();
}

void PaperDispenser::stop() {
    if (m_bDispensing) {
        m_bStopped = true;
    }
}

void PaperDispenser::rampDown() {
    if (!m_bRampingDown) {
        m_eState = RAMPING_DOWN;
        m_bRampingDown = true;
        m_ulRampDownStartTime = millis();
        dcMotorReverse();
    }
}

void PaperDispenser::setStepperSteps(int steps) {
    if (steps > 0) {
        m_nStepperSteps = steps;
    }
}

bool PaperDispenser::isLimitSwitchPressed() {
    return digitalRead(m_nLimitSwitchPin) == HIGH;
}

bool PaperDispenser::isDispensing() {
    return m_bDispensing;
}

void PaperDispenser::dcMotorForward() {
    digitalWrite(m_nIn1Pin, HIGH);
    digitalWrite(m_nIn2Pin, LOW);
}

void PaperDispenser::dcMotorReverse() {
    digitalWrite(m_nIn1Pin, LOW);
    digitalWrite(m_nIn2Pin, HIGH);
}

void PaperDispenser::dcMotorStop() {
    digitalWrite(m_nIn1Pin, LOW);
    digitalWrite(m_nIn2Pin, LOW);
}

void PaperDispenser::waitForLimitSwitch() {
    unsigned long startTime = millis();
    
    while (!isLimitSwitchPressed()) {
        // Send status every second
        if (millis() - m_ulLastStatusTime >= k_ulStatusInterval) {
            sendStatusJson();
            m_ulLastStatusTime = millis();
        }
        
        // Check for timeout (10 seconds)
        if (millis() - startTime > 10000) {
            sendErrorJson("timeout", "Limit switch not triggered after timeout");
            break;
        }
        
        delay(100);
    }
}

void PaperDispenser::handleSerial(const String& line) {
    // Try to parse as JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, line);
    
    if (!error) {
        // Check if this is a valid JSON command for PaperDispenser
        if (doc.containsKey("v") && doc.containsKey("target") && doc.containsKey("cmd")) {
            
            // Check if it's for PaperDispenser and it's the right version
            if (doc["v"] == 1 && strcmp(doc["target"], "PaperDispenser") == 0) {
                processJsonCommand(doc);
                return;
            }
        }
    }
    
    // No fallback to legacy commands for this module as it's new
}

void PaperDispenser::processJsonCommand(JsonDocument& doc) {
    const char* cmdName = doc["cmd"];
    
    // Check if name field is present and matches this instance
    if (!doc.containsKey("name")) {
        sendErrorJson("missing_parameter", "Name parameter is required to identify the dispenser");
        return;
    }
    
    // Only process commands for this specific dispenser instance
    const char* targetName = doc["name"];
    if (strcmp(targetName, m_strName.c_str()) != 0) {
        // Command is for a different dispenser, ignore it
        return;
    }
    
    if (strcmp(cmdName, "dispense") == 0) {
        if (doc.containsKey("value")) {
            int amount = doc["value"];
            dispense(amount);
            sendAckJson("dispense", true, amount, "started");
        } else {
            // Missing value parameter
            sendErrorJson("missing_parameter", "Value parameter is required for dispense command");
        }
    } 
    else if (strcmp(cmdName, "stop") == 0) {
        stop();
        sendAckJson("stop");
    } 
    else if (strcmp(cmdName, "setStepperSteps") == 0) {
        if (doc.containsKey("value")) {
            int steps = doc["value"];
            setStepperSteps(steps);
            sendAckJson("setStepperSteps", true, steps);
        } else {
            // Missing value parameter
            sendErrorJson("missing_parameter", "Value parameter is required for setStepperSteps command");
        }
    }
    else {
        // Unknown command
        sendErrorJson("unknown_command", "Command not recognized");
    }
}

void PaperDispenser::sendStatusJson() {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "PaperDispenser";
    doc["type"] = "status";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strName;  // Include name of this dispenser instance
    
    // Set status based on current state
    const char* statusText;
    switch (m_eState) {
        case IDLE:      statusText = "idle"; break;
        case HOMING:    statusText = "homing"; break;
        case DISPENSING: statusText = "in_progress"; break;
        case RAMPING_DOWN: statusText = "ramping_down"; break;
        case COMPLETE:  statusText = "complete"; break;
        case ERROR:     statusText = "error"; break;
        default:        statusText = "unknown";
    }
    
    if (m_eState != IDLE) {
        data["action"] = "dispense";
        data["current"] = m_nCurrentPaper;
        data["total"] = m_nTotalPapers;
    }
    
    data["status"] = statusText;
    
    serializeJson(doc, Serial);
    Serial.println();
}

void PaperDispenser::sendEventJson(const char* event) {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "PaperDispenser";
    doc["type"] = "event";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strName;  // Include name of this dispenser instance
    data["event"] = event;
    
    if (m_nTotalPapers > 0) {
        data["total"] = m_nTotalPapers;
    }
    
    serializeJson(doc, Serial);
    Serial.println();
}

void PaperDispenser::sendErrorJson(const char* errorType, const char* details) {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "PaperDispenser";
    doc["type"] = "error";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strName;  // Include name of this dispenser instance
    data["error"] = errorType;
    data["details"] = details;
    
    if (m_bDispensing) {
        data["current"] = m_nCurrentPaper;
        data["total"] = m_nTotalPapers;
    }
    
    serializeJson(doc, Serial);
    Serial.println();
}

void PaperDispenser::sendAckJson(const char* action, bool ok, int value, const char* status) {
    StaticJsonDocument<256> doc;
    
    doc["v"] = 1;
    doc["source"] = "PaperDispenser";
    doc["type"] = "ack";
    doc["ts"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["name"] = m_strName;  // Include name of this dispenser instance
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