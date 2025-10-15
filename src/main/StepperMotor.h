#ifndef STEPPERMOTOR_H
#define STEPPERMOTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Direction constants
#define CW  1
#define CCW 0

class StepperMotor {
public:
    StepperMotor(uint8_t pulsePin, uint8_t dirPin);
    void begin();
    void rotate(int steps, uint8_t direction, unsigned int delayMicros = 1000);
    void setSpeed(unsigned int delayMicros);
    
private:
    uint8_t m_nPulsePin;
    uint8_t m_nDirPin;
    unsigned int m_ulStepDelay;
    
    void step();
};

/**
 * PaperDispenser - Controls paper dispensing mechanism
 * 
 * JSON Command Structure (Raspberry Pi → Arduino):
 * {
 *   "v": 1,                          // Protocol version
 *   "target": "PaperDispenser",      // Must be "PaperDispenser" for this module
 *   "cmd": "dispense",               // Command: "dispense", "stop", "setStepperSteps"
 *   "name": "PaperDispenser1",       // Name of the specific paper dispenser instance to target
 *   "value": 5                       // For dispense: number of papers, for setStepperSteps: step count
 * }
 * 
 * Example commands:
 * {"v":1,"target":"PaperDispenser","cmd":"dispense","name":"PaperDispenser1","value":5}
 * {"v":1,"target":"PaperDispenser","cmd":"stop","name":"PaperDispenser1"}
 * {"v":1,"target":"PaperDispenser","cmd":"setStepperSteps","name":"PaperDispenser1","value":1200}
 */
class PaperDispenser {
public:
    PaperDispenser(uint8_t pulsePin, uint8_t dirPin, uint8_t in1Pin, uint8_t in2Pin, 
                   uint8_t enPin, uint8_t limitSwitchPin, int stepperSteps = 1000,
                   String name = "PaperDispenser");
    void begin();
    void update();
    
    // Control methods
    void dispense(int numOfPapers);
    void stop();
    void rampDown();
    void setStepperSteps(int steps);
    bool isLimitSwitchPressed();
    bool isDispensing();
    
    // Status flag methods
    bool hasStatusUpdate() const;
    bool hasEvent() const;
    bool hasError() const;
    void clearStatusUpdate();
    void clearEvent();
    void clearError();
    
    // State machine forward declaration
    enum DispenserState {
        IDLE,
        HOMING,
        DISPENSING,
        RAMPING_DOWN,
        COMPLETE,
        ERROR
    };
    
    // Status accessors
    String getName() const;
    String getLastEvent() const;
    String getLastError() const;
    String getLastErrorDetails() const;
    DispenserState getState() const;
    int getCurrentPaper() const;
    int getTotalPapers() const;
    
    // New command processing method
    bool processCommand(const String& line, JsonDocument& doc);
    
private:
    // Hardware components
    StepperMotor m_stepper;
    String m_strName;
    
    // Hardware pins
    uint8_t m_nIn1Pin;
    uint8_t m_nIn2Pin;
    uint8_t m_nEnPin;
    uint8_t m_nLimitSwitchPin;
    
    // Operational parameters
    int m_nStepperSteps;
    int m_nCurrentPaper;
    int m_nTotalPapers;
    
    // State flags
    bool m_bDispensing;
    bool m_bRampingDown;
    bool m_bOperationComplete;
    bool m_bStopped;
    
    // Status tracking flags
    bool m_bStatusUpdated;     // Indicates a status update is available
    bool m_bEventOccurred;     // Indicates an event has occurred
    bool m_bErrorOccurred;     // Indicates an error has occurred
    String m_strLastEvent;     // The last event that occurred
    String m_strLastError;     // The last error that occurred
    String m_strLastErrorDetails; // Details about the last error
    
    // Timing variables (milliseconds)
    unsigned long m_ulLastStatusTime;
    unsigned long m_ulOperationStartTime;
    unsigned long m_ulRampDownStartTime;
    
    // Constants (milliseconds)
    const unsigned long k_ulStatusInterval = 1000;    // Status update interval (1s)
    const unsigned long k_ulRampDownTime = 8000;      // Ramp down time (8s)
    
    // Motor control
    void dcMotorForward();
    void dcMotorReverse();
    void dcMotorStop();
    void waitForLimitSwitch();
    
    // State machine instance
    DispenserState m_eState;
    void updateStateMachine();
    
    // JSON message helpers - now set flags instead of writing to Serial
    void setStatusUpdated();
    void setEvent(const String& event);
    void setError(const String& errorType, const String& details);
    
    // JSON command handling
    void processJsonCommand(JsonDocument& doc);
};

#endif