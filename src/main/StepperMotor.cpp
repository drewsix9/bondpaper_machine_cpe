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
    
    // Initialize status tracking flags
    m_bStatusUpdated = false;
    m_bEventOccurred = false;
    m_bErrorOccurred = false;
    m_strLastEvent = "";
    m_strLastError = "";
    m_strLastErrorDetails = "";
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
    
    // Set status flag for initial status
    setStatusUpdated();
}

void PaperDispenser::update() {
    // Update state machine
    updateStateMachine();
    
    // Set status flag for periodic status updates while active
    if (m_eState != IDLE && m_eState != COMPLETE && 
        (millis() - m_ulLastStatusTime >= k_ulStatusInterval)) {
        setStatusUpdated();
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
                
                // Set status flag for updated status
                setStatusUpdated();
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
                
                // Set event flag for completion
                setEvent("dispense_complete");
                setStatusUpdated();
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
        setError("invalid_parameter", "Number of papers must be greater than zero");
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
    
    // Set status flag for initial status
    setStatusUpdated();
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

// Status flag accessor methods
bool PaperDispenser::hasStatusUpdate() const {
    return m_bStatusUpdated;
}

bool PaperDispenser::hasEvent() const {
    return m_bEventOccurred;
}

bool PaperDispenser::hasError() const {
    return m_bErrorOccurred;
}

void PaperDispenser::clearStatusUpdate() {
    m_bStatusUpdated = false;
}

void PaperDispenser::clearEvent() {
    m_bEventOccurred = false;
}

void PaperDispenser::clearError() {
    m_bErrorOccurred = false;
}

String PaperDispenser::getName() const {
    return m_strName;
}

String PaperDispenser::getLastEvent() const {
    return m_strLastEvent;
}

String PaperDispenser::getLastError() const {
    return m_strLastError;
}

String PaperDispenser::getLastErrorDetails() const {
    return m_strLastErrorDetails;
}

PaperDispenser::DispenserState PaperDispenser::getState() const {
    return m_eState;
}

int PaperDispenser::getCurrentPaper() const {
    return m_nCurrentPaper;
}

int PaperDispenser::getTotalPapers() const {
    return m_nTotalPapers;
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
        // Set status flag every second
        if (millis() - m_ulLastStatusTime >= k_ulStatusInterval) {
            setStatusUpdated();
            m_ulLastStatusTime = millis();
        }
        
        // Check for timeout (10 seconds)
        if (millis() - startTime > 10000) {
            setError("timeout", "Limit switch not triggered after timeout");
            break;
        }
        
        delay(100);
    }
}

bool PaperDispenser::processCommand(const String& line, JsonDocument& doc) {
    // Check if the command is already parsed as JSON
    bool isParsed = (doc.size() > 0);
    
    if (!isParsed) {
        // Try to parse as JSON
        DeserializationError error = deserializeJson(doc, line);
        if (error) {
            return false; // Not valid JSON
        }
    }
    
    // Check if this is a valid JSON command for PaperDispenser
    if (doc.containsKey("v") && doc.containsKey("target") && doc.containsKey("cmd")) {
        // Check if it's for PaperDispenser and it's the right version
        if (doc["v"] == 1 && strcmp(doc["target"], "PaperDispenser") == 0) {
            // Only process commands for this specific dispenser instance if name is present
            if (doc.containsKey("name")) {
                const char* targetName = doc["name"];
                if (strcmp(targetName, m_strName.c_str()) == 0) {
                    processJsonCommand(doc);
                    return true;
                }
                return false; // Command was for a different dispenser
            } else {
                // No name specified, assume it's for this dispenser
                processJsonCommand(doc);
                return true;
            }
        }
    }
    
    return false; // Not handled
}

void PaperDispenser::processJsonCommand(JsonDocument& doc) {
    const char* cmdName = doc["cmd"];
    
    // We already checked for name in processCommand, so here we can just process the command
    if (strcmp(cmdName, "dispense") == 0) {
        if (doc.containsKey("value")) {
            int amount = doc["value"];
            dispense(amount);
            // Status update will be sent from main.ino
        } else {
            // Missing value parameter
            setError("missing_parameter", "Value parameter is required for dispense command");
        }
    } 
    else if (strcmp(cmdName, "stop") == 0) {
        stop();
        // Status update will be sent from main.ino
    } 
    else if (strcmp(cmdName, "setStepperSteps") == 0) {
        if (doc.containsKey("value")) {
            int steps = doc["value"];
            setStepperSteps(steps);
            // Status update will be sent from main.ino
        } else {
            // Missing value parameter
            setError("missing_parameter", "Value parameter is required for setStepperSteps command");
        }
    }
    else {
        // Unknown command
        setError("unknown_command", "Command not recognized");
    }
}

void PaperDispenser::setStatusUpdated() {
    m_bStatusUpdated = true;
}

void PaperDispenser::setEvent(const String& event) {
    m_bEventOccurred = true;
    m_strLastEvent = event;
}

void PaperDispenser::setError(const String& errorType, const String& details) {
    m_bErrorOccurred = true;
    m_strLastError = errorType;
    m_strLastErrorDetails = details;
}

// Removed sendAckJson - acknowledgments will be handled by main.ino