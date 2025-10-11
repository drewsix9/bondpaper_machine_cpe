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
    void handleSerial(const String& line);
    
    // Control methods
    void dispense(int numOfPapers);
    void stop();
    void rampDown();
    void setStepperSteps(int steps);
    bool isLimitSwitchPressed();
    bool isDispensing();
    
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
    
    // State machine
    enum DispenserState {
        IDLE,
        HOMING,
        DISPENSING,
        RAMPING_DOWN,
        COMPLETE,
        ERROR
    };
    DispenserState m_eState;
    void updateStateMachine();
    
    // JSON message helpers
    void sendStatusJson();
    void sendEventJson(const char* event);
    void sendErrorJson(const char* errorType, const char* details);
    void sendAckJson(const char* action, bool ok = true, int value = 0, const char* status = nullptr);
    
    // JSON command handling
    void processJsonCommand(JsonDocument& doc);
};

#endif