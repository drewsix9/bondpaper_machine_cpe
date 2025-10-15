/**
 * Main controller for Bond Paper Machine
 * 
 * This sketch integrates several modules:
 * - CoinSlotISR: Detects inserted coins with interrupt
 * - CoinCounter: Tracks and dispenses coins of different denominations
 * - PaperDispenser: Controls paper dispensing using stepper and DC motors
 * - RelayHopper: Controls relay outputs for coin dispensers
 * 
 * The system accepts JSON commands over serial from a Raspberry Pi
 * and returns JSON responses for status, events, and acknowledgements.
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include "CoinSlotISR.h"
#include "CoinCounter.h"
#include "StepperMotor.h"
#include "config.h"
#include "SerialComm.h"
// RelayHopper.h is now included through CoinCounter.h

// ========== SYSTEM CONFIGURATION ==========
// All pin definitions are now directly used from config.h
// - Coin slot uses COINS_PULSE_PIN
// - A4 paper dispenser uses a4_step_motors structure
// - Long paper dispenser uses long_step_motors structure 
// - Coin counters use ONE_SENSOR_PIN, FIVE_SENSOR_PIN, and TEN_SENSOR_PIN
// - Relay control uses RELAY1_PIN, RELAY2_PIN, and RELAY3_PIN
// - Buzzer functionality uses BUZZER_PIN

  // ========== MODULE INSTANCES ==========

// Coin Slot (detects inserted coins)
CoinSlotISR coinSlot(COINS_PULSE_PIN);// Paper Dispensers
PaperDispenser paperDispenser1(
  a4_step_motors.stepper.pulse_pin, a4_step_motors.stepper.dir_pin,
  a4_step_motors.dc_motor.IN1, a4_step_motors.dc_motor.IN2, a4_step_motors.dc_motor.en_pin,
  a4_step_motors.stepper.limit_switch, a4_step_motors.stepper.steps, "a4Dispenser"
);

PaperDispenser paperDispenser2(
  long_step_motors.stepper.pulse_pin, long_step_motors.stepper.dir_pin,
  long_step_motors.dc_motor.IN1, long_step_motors.dc_motor.IN2, long_step_motors.dc_motor.en_pin,
  long_step_motors.stepper.limit_switch, long_step_motors.stepper.steps, "longDispenser"
);

// Relay Control (coin hopper control) - created first so we can pass to coin counters
RelayHopper relayController(RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, "RelayController");

// Coin Counters (for dispensing coins) - each connected to a specific relay
CoinCounter oneCounter(ONE_SENSOR_PIN, "Peso1", &relayController, 1);
CoinCounter fiveCounter(FIVE_SENSOR_PIN, "Peso5", &relayController, 2);
CoinCounter tenCounter(TEN_SENSOR_PIN, "Peso10", &relayController, 3);

// ========== SERIAL COMMUNICATION ==========
const long SERIAL_BAUD = 115200;  // High baud rate for fast communication
const int BUFFER_SIZE = 256;      // Serial buffer size
char serialBuffer[BUFFER_SIZE];   // Buffer for incoming serial data
int bufferIndex = 0;              // Current position in buffer

// JSON documents for system status and commands
StaticJsonDocument<512> systemInfoDoc;
StaticJsonDocument<512> commandDoc;

// ========== TIMING VARIABLES ==========
unsigned long lastStatusTime = 0;
const unsigned long STATUS_INTERVAL = 5000;  // Status update every 5 seconds

// ========== SYSTEM FUNCTIONS ==========

/**
 * Send a system-level acknowledgement for commands that 
 * aren't handled by specific modules
 */
void sendSystemAck(const char* cmd, bool ok, const char* status = nullptr) {
  SerialComm::sendSystemAck(cmd, ok, status);
}

/**
 * Send a system status message with all component states
 */
void sendSystemStatusJson() {
  systemInfoDoc.clear();
  
  systemInfoDoc["v"] = 1;
  systemInfoDoc["source"] = "System";
  systemInfoDoc["type"] = "status";
  systemInfoDoc["ts"] = millis();
  
  JsonObject data = systemInfoDoc.createNestedObject("data");
  
  // Coin slot status
  JsonObject coinSlotObj = data.createNestedObject("coinSlot");
  coinSlotObj["attached"] = coinSlot.isAttached();
  
  // Paper dispenser status
  JsonObject paperObj = data.createNestedObject("paperDispensers");
  paperObj["paper1"] = paperDispenser1.isDispensing() ? "dispensing" : "idle";
  paperObj["paper2"] = paperDispenser2.isDispensing() ? "dispensing" : "idle";
  
  // Coin counters status
  JsonObject countersObj = data.createNestedObject("coinCounters");
  countersObj["p1"] = oneCounter.getCount();
  countersObj["p5"] = fiveCounter.getCount();
  countersObj["p10"] = tenCounter.getCount();
  
  countersObj["p1_dispensing"] = oneCounter.isDispensing();
  countersObj["p5_dispensing"] = fiveCounter.isDispensing();
  countersObj["p10_dispensing"] = tenCounter.isDispensing();
  
  // Relay status
  JsonObject relaysObj = data.createNestedObject("relays");
  // We use inverse logic where false = ON, true = OFF for relays
  relaysObj["relay1"] = relayController.getRelayState(1) ? "OFF" : "ON";
  relaysObj["relay2"] = relayController.getRelayState(2) ? "OFF" : "ON";
  relaysObj["relay3"] = relayController.getRelayState(3) ? "OFF" : "ON";
  
  // Create a message and send through SerialComm
  SerialMessage message("System", "status", &systemInfoDoc);
  SerialComm::sendMessage(message);
}

/**
 * Process a system-level command (not specific to any one module)
 */
void processSystemCommand(JsonDocument& doc) {
  const char* cmd = doc["cmd"];
  
  if (strcmp(cmd, "status") == 0) {
    sendSystemStatusJson();
    sendSystemAck("status", true);
  }
  else if (strcmp(cmd, "reset") == 0) {
    resetSystem();
    sendSystemStatusJson();
    sendSystemAck("reset", true);
  }
  else {
    // Unknown system command
    sendSystemAck(cmd, false, "unknown_command");
  }
}

/**
 * Reset the entire system to initial state
 */
void resetSystem() {
  // Reset all modules
  coinSlot.detach();
  
  oneCounter.reset();
  fiveCounter.reset();
  tenCounter.reset();
  
  // Stop any ongoing operations
  oneCounter.stopDispensing();
  fiveCounter.stopDispensing();
  tenCounter.stopDispensing();
  
  relayController.setRelay(1, true); // Turn OFF relay 1
  relayController.setRelay(2, true); // Turn OFF relay 2
  relayController.setRelay(3, true); // Turn OFF relay 3
  
  // Can't reset paper dispensers if they're mid-operation
  // but they'll complete their current operation
}

/**
 * Process an incoming serial line
 */
void processSerialLine(const String& line) {
  if (line.length() == 0) return;
  
  // Try parsing as JSON first
  commandDoc.clear();
  DeserializationError error = deserializeJson(commandDoc, line);
  
  if (!error && commandDoc.containsKey("v") && commandDoc.containsKey("target") && commandDoc.containsKey("cmd")) {
    // Valid JSON command - route to appropriate module
    const char* target = commandDoc["target"];
    bool handled = false;
    
    if (strcmp(target, "CoinSlot") == 0) {
      handled = coinSlot.processCommand(line, commandDoc);
      
      if (handled) {
        const char* cmd = commandDoc["cmd"];
        
        if (strcmp(cmd, "reset") == 0) {
          SerialComm::sendSystemAck("reset", true);
        }
        else if (strcmp(cmd, "attach") == 0) {
          SerialComm::sendSystemAck("attach", true, coinSlot.isAttached() ? "attached" : "already_attached");
        }
        else if (strcmp(cmd, "detach") == 0) {
          SerialComm::sendSystemAck("detach", true, "detached");
        }
        else if (strcmp(cmd, "get") == 0 || strcmp(cmd, "status") == 0) {
          // Status update will be sent from main loop when flag is checked
          SerialComm::sendSystemAck(cmd, true);
        }
      } else {
        SerialComm::sendSystemAck("unknown_cmd", false);
      }
    }
    else if (strcmp(target, "PaperDispenser") == 0) {
      // Try both dispensers with the new processCommand method
      handled = paperDispenser1.processCommand(line, commandDoc) ||
                paperDispenser2.processCommand(line, commandDoc);
                
      // Send acknowledgment based on the command result
      if (handled && commandDoc.containsKey("cmd") && commandDoc.containsKey("name")) {
        const char* cmd = commandDoc["cmd"];
        const char* name = commandDoc["name"];
        
        if (strcmp(cmd, "dispense") == 0 && commandDoc.containsKey("value")) {
          int amount = commandDoc["value"];
          SerialComm::sendPaperDispenserAck(name, "dispense", true, amount, "started");
        }
        else if (strcmp(cmd, "stop") == 0) {
          SerialComm::sendPaperDispenserAck(name, "stop", true);
        }
        else if (strcmp(cmd, "setStepperSteps") == 0 && commandDoc.containsKey("value")) {
          int steps = commandDoc["value"];
          SerialComm::sendPaperDispenserAck(name, "setStepperSteps", true, steps);
        }
      }
      else if (!handled && commandDoc.containsKey("name") && commandDoc.containsKey("cmd")) {
        // Command was for PaperDispenser but not handled
        const char* name = commandDoc["name"];
        const char* cmd = commandDoc["cmd"];
        
        if (strcmp(cmd, "dispense") == 0 && !commandDoc.containsKey("value")) {
          SerialComm::sendPaperDispenserError(name, "missing_parameter", "Value parameter is required", 0, 0);
        }
        else {
          SerialComm::sendPaperDispenserAck(name, cmd, false, 0, "unknown_command");
        }
      }
    }
    else if (strcmp(target, "CoinCounter") == 0) {
      // Try all coin counters with the updated processCommand method
      handled = oneCounter.processCommand(line, commandDoc) ||
                fiveCounter.processCommand(line, commandDoc) ||
                tenCounter.processCommand(line, commandDoc);
                
      // Send acknowledgment based on the command result
      if (handled && commandDoc.containsKey("cmd") && commandDoc.containsKey("name")) {
        const char* cmd = commandDoc["cmd"];
        const char* name = commandDoc["name"];
        
        if (strcmp(cmd, "reset") == 0) {
          SerialComm::sendCoinCounterAck(name, "reset", true);
        }
        else if (strcmp(cmd, "dispense") == 0 && commandDoc.containsKey("value")) {
          int amount = commandDoc["value"];
          SerialComm::sendCoinCounterAck(name, "dispense", true, amount, "started");
        }
        else if (strcmp(cmd, "stop") == 0) {
          SerialComm::sendCoinCounterAck(name, "stop", true);
        }
      }
      else if (!handled && commandDoc.containsKey("name") && commandDoc.containsKey("cmd")) {
        // Command was for CoinCounter but not handled
        const char* name = commandDoc["name"];
        const char* cmd = commandDoc["cmd"];
        
        if (strcmp(cmd, "dispense") == 0 && !commandDoc.containsKey("value")) {
          SerialComm::sendCoinCounterAck(name, "dispense", false, 0, "missing_value");
        }
        else {
          SerialComm::sendCoinCounterAck(name, cmd, false, 0, "unknown_command");
        }
      }
    }
    else if (strcmp(target, "Relay") == 0) {
      handled = relayController.processCommand(line, commandDoc);
      
      if (handled) {
        const char* cmd = commandDoc["cmd"];
        
        if (strcmp(cmd, "setRelay") == 0) {
          int relayNum = commandDoc["value"];
          const char* stateStr = commandDoc["state"];
          bool state = (strcmp(stateStr, "off") == 0);  // ON = false, OFF = true
          
          // Send ACK through SerialComm
          StaticJsonDocument<128> relayData;
          relayData["relay"] = relayNum;
          relayData["action"] = "set";
          relayData["state"] = state ? "OFF" : "ON";
          relayData["ok"] = true;
          
          SerialMessage relayMessage("Relay", "ack", &relayData);
          SerialComm::sendMessage(relayMessage);
        }
        else if (strcmp(cmd, "get") == 0) {
          // Status will be sent from the main loop when checking for status updates
        }
      }
      else {
        // Command failed
        const char* cmd = commandDoc["cmd"];
        if (strcmp(cmd, "setRelay") == 0) {
          int relayNum = commandDoc.containsKey("value") ? commandDoc["value"] : 0;
          
          if (!commandDoc.containsKey("value") || !commandDoc.containsKey("state")) {
            SerialComm::sendError("Relay", "setRelay", "missing_parameters");
          }
          else {
            const char* stateStr = commandDoc["state"];
            if (strcmp(stateStr, "on") != 0 && strcmp(stateStr, "off") != 0) {
              SerialComm::sendError("Relay", "setRelay", "invalid_state");
            }
          }
        }
        else {
          SerialComm::sendError("Relay", cmd, "unknown_command");
        }
      }
    }
    else if (strcmp(target, "System") == 0) {
      processSystemCommand(commandDoc);
      handled = true;
    }
    
    if (!handled) {
      sendSystemAck("unknown_target", false, target);
    }
    
    return;
  }
  
  // Not JSON or invalid JSON - try legacy commands
  if (line == "status") {
    sendSystemStatusJson();
  }
  else if (line == "reset") {
    resetSystem();
    sendSystemStatusJson();
  }
  else {
    // Try letting each module handle the command
    // For CoinSlot, use the new approach
    commandDoc.clear();
    coinSlot.processCommand(line, commandDoc);
    
    // For PaperDispenser, use the new approach
    commandDoc.clear();
    paperDispenser1.processCommand(line, commandDoc);
    paperDispenser2.processCommand(line, commandDoc);
    
    // For CoinCounter, use the new approach
    commandDoc.clear();
    oneCounter.processCommand(line, commandDoc);
    fiveCounter.processCommand(line, commandDoc);
    tenCounter.processCommand(line, commandDoc);
    
    // For RelayHopper, use the new approach
    commandDoc.clear();
    relayController.processCommand(line, commandDoc);
  }
}

/**
 * Print current system status to Serial (for debugging)
 */
void printSystemStatus() {
  // Just send the system status JSON instead of custom format
  // This ensures consistent output format
  sendSystemStatusJson();
}

// ========== MAIN ARDUINO FUNCTIONS ==========

void setup() {
  // Initialize serial communication through SerialComm
  SerialComm::begin(SERIAL_BAUD);
  
  // Create and send boot event
  StaticJsonDocument<64> bootData;
  bootData["event"] = "boot";
  SerialMessage bootMessage("System", "event", &bootData);
  SerialComm::sendMessage(bootMessage);
  
  // Setup buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize modules
  coinSlot.begin();
  // Don't automatically attach the interrupt - wait for command
  
  // Short beep to indicate system is starting
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  
  paperDispenser1.begin();
  paperDispenser2.begin();
  
  oneCounter.begin();
  fiveCounter.begin();
  tenCounter.begin();
  relayController.begin();
  
  // Send initial status
  sendSystemStatusJson();
}

void loop() {
  // Read and process serial commands
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Add to buffer if not newline and we have space
    if (c != '\n' && bufferIndex < BUFFER_SIZE - 1) {
      serialBuffer[bufferIndex++] = c;
    }
    // Process line when we get newline or buffer is full
    else {
      serialBuffer[bufferIndex] = '\0'; // Null-terminate
      String line = String(serialBuffer);
      processSerialLine(line);
      bufferIndex = 0; // Reset buffer
    }
  }
  
  // Update all modules
  coinSlot.update();
  paperDispenser1.update();
  paperDispenser2.update();
  oneCounter.update();
  fiveCounter.update();
  tenCounter.update();
  relayController.update();
  
  // Check for status updates from CoinSlot module
  if (coinSlot.hasStatusUpdate()) {
    StaticJsonDocument<128> coinSlotData;
    coinSlotData["totalValue"] = coinSlot.getTotalValue();
    coinSlotData["attached"] = coinSlot.isAttached();
    
    SerialMessage coinSlotMessage("CoinSlot", "status", &coinSlotData);
    SerialComm::sendMessage(coinSlotMessage);
    
    coinSlot.clearStatusUpdate();
  }
  
  // Check for events from CoinSlot module
  if (coinSlot.hasEvent()) {
    StaticJsonDocument<128> coinEventData;
    coinEventData["coinValue"] = coinSlot.getLastCoinValue();
    coinEventData["totalValue"] = coinSlot.getTotalValue();
    
    SerialMessage coinEventMessage("CoinSlot", "event", &coinEventData);
    SerialComm::sendMessage(coinEventMessage);
    
    coinSlot.clearEvent();
  }
  
  // Check for status updates from CoinCounter modules and send centralized messages
  if (oneCounter.hasStatusUpdate()) {
    SerialComm::sendCoinCounterStatus(
      oneCounter.getName().c_str(),
      oneCounter.getCurrentCount(),
      oneCounter.getTargetCount(),
      oneCounter.isDispensing()
    );
    oneCounter.clearStatusUpdate();
  }
  
  if (fiveCounter.hasStatusUpdate()) {
    SerialComm::sendCoinCounterStatus(
      fiveCounter.getName().c_str(),
      fiveCounter.getCurrentCount(),
      fiveCounter.getTargetCount(),
      fiveCounter.isDispensing()
    );
    fiveCounter.clearStatusUpdate();
  }
  
  if (tenCounter.hasStatusUpdate()) {
    SerialComm::sendCoinCounterStatus(
      tenCounter.getName().c_str(),
      tenCounter.getCurrentCount(),
      tenCounter.getTargetCount(),
      tenCounter.isDispensing()
    );
    tenCounter.clearStatusUpdate();
  }
  
  // Check for status updates from RelayHopper module
  if (relayController.hasStatusUpdate()) {
    StaticJsonDocument<256> relayData;
    
    // Add relay states to the data document
    JsonArray relayStates = relayData.createNestedArray("states");
    // Fixed: using hardcoded 3 instead of getRelayCount since we have exactly 3 relays
    for (int i = 1; i <= 3; i++) {
      JsonObject relay = relayStates.createNestedObject();
      relay["relay"] = i;
      relay["state"] = relayController.getRelayState(i) ? "OFF" : "ON";  // ON = false, OFF = true
    }
    
    SerialMessage relayMessage("Relay", "status", &relayData);
    SerialComm::sendMessage(relayMessage);
    
    relayController.clearStatusUpdate();
  }
  
  // Check for events from CoinCounter modules
  if (oneCounter.hasEvent()) {
    if (oneCounter.hasReachedTarget()) {
      SerialComm::sendCoinCounterEvent(
        oneCounter.getName().c_str(),
        "target_reached",
        oneCounter.getCurrentCount(),
        oneCounter.getTargetCount()
      );
    } else if (oneCounter.hasTimedOut()) {
      SerialComm::sendCoinCounterTimeout(
        oneCounter.getName().c_str(),
        oneCounter.getCurrentCount(),
        oneCounter.getTargetCount()
      );
    }
    oneCounter.clearEvent();
  }
  
  if (fiveCounter.hasEvent()) {
    if (fiveCounter.hasReachedTarget()) {
      SerialComm::sendCoinCounterEvent(
        fiveCounter.getName().c_str(),
        "target_reached",
        fiveCounter.getCurrentCount(),
        fiveCounter.getTargetCount()
      );
    } else if (fiveCounter.hasTimedOut()) {
      SerialComm::sendCoinCounterTimeout(
        fiveCounter.getName().c_str(),
        fiveCounter.getCurrentCount(),
        fiveCounter.getTargetCount()
      );
    }
    fiveCounter.clearEvent();
  }
  
  if (tenCounter.hasEvent()) {
    if (tenCounter.hasReachedTarget()) {
      SerialComm::sendCoinCounterEvent(
        tenCounter.getName().c_str(),
        "target_reached",
        tenCounter.getCurrentCount(),
        tenCounter.getTargetCount()
      );
    } else if (tenCounter.hasTimedOut()) {
      SerialComm::sendCoinCounterTimeout(
        tenCounter.getName().c_str(),
        tenCounter.getCurrentCount(),
        tenCounter.getTargetCount()
      );
    }
    tenCounter.clearEvent();
  }
  
  // Check for status updates from PaperDispenser modules
  if (paperDispenser1.hasStatusUpdate()) {
    const char* statusText;
    switch (paperDispenser1.getState()) {
      case PaperDispenser::IDLE:      statusText = "idle"; break;
      case PaperDispenser::HOMING:    statusText = "homing"; break;
      case PaperDispenser::DISPENSING: statusText = "in_progress"; break;
      case PaperDispenser::RAMPING_DOWN: statusText = "ramping_down"; break;
      case PaperDispenser::COMPLETE:  statusText = "complete"; break;
      case PaperDispenser::ERROR:     statusText = "error"; break;
      default:        statusText = "unknown";
    }
    
    SerialComm::sendPaperDispenserStatus(
      paperDispenser1.getName().c_str(),
      statusText,
      paperDispenser1.getCurrentPaper(),
      paperDispenser1.getTotalPapers()
    );
    paperDispenser1.clearStatusUpdate();
  }
  
  if (paperDispenser2.hasStatusUpdate()) {
    const char* statusText;
    switch (paperDispenser2.getState()) {
      case PaperDispenser::IDLE:      statusText = "idle"; break;
      case PaperDispenser::HOMING:    statusText = "homing"; break;
      case PaperDispenser::DISPENSING: statusText = "in_progress"; break;
      case PaperDispenser::RAMPING_DOWN: statusText = "ramping_down"; break;
      case PaperDispenser::COMPLETE:  statusText = "complete"; break;
      case PaperDispenser::ERROR:     statusText = "error"; break;
      default:        statusText = "unknown";
    }
    
    SerialComm::sendPaperDispenserStatus(
      paperDispenser2.getName().c_str(),
      statusText,
      paperDispenser2.getCurrentPaper(),
      paperDispenser2.getTotalPapers()
    );
    paperDispenser2.clearStatusUpdate();
  }
  
  // Check for events from PaperDispenser modules
  if (paperDispenser1.hasEvent()) {
    SerialComm::sendPaperDispenserEvent(
      paperDispenser1.getName().c_str(),
      paperDispenser1.getLastEvent().c_str(),
      paperDispenser1.getTotalPapers()
    );
    paperDispenser1.clearEvent();
  }
  
  if (paperDispenser2.hasEvent()) {
    SerialComm::sendPaperDispenserEvent(
      paperDispenser2.getName().c_str(),
      paperDispenser2.getLastEvent().c_str(),
      paperDispenser2.getTotalPapers()
    );
    paperDispenser2.clearEvent();
  }
  
  // Check for errors from PaperDispenser modules
  if (paperDispenser1.hasError()) {
    SerialComm::sendPaperDispenserError(
      paperDispenser1.getName().c_str(),
      paperDispenser1.getLastError().c_str(),
      paperDispenser1.getLastErrorDetails().c_str(),
      paperDispenser1.getCurrentPaper(),
      paperDispenser1.getTotalPapers()
    );
    paperDispenser1.clearError();
  }
  
  if (paperDispenser2.hasError()) {
    SerialComm::sendPaperDispenserError(
      paperDispenser2.getName().c_str(),
      paperDispenser2.getLastError().c_str(),
      paperDispenser2.getLastErrorDetails().c_str(),
      paperDispenser2.getCurrentPaper(),
      paperDispenser2.getTotalPapers()
    );
    paperDispenser2.clearError();
  }
  
  // Periodically send system status during activity
  // Only if any subsystem is active to avoid flooding the serial
  bool systemActive = coinSlot.isAttached() || 
                      paperDispenser1.isDispensing() || 
                      paperDispenser2.isDispensing() || 
                      oneCounter.isDispensing() ||
                      fiveCounter.isDispensing() ||
                      tenCounter.isDispensing() ||
                      relayController.hasStatusUpdate() ||
                      paperDispenser1.hasStatusUpdate() ||
                      paperDispenser2.hasStatusUpdate();
                      
  if (systemActive && (millis() - lastStatusTime >= STATUS_INTERVAL)) {
    sendSystemStatusJson();
    lastStatusTime = millis();
  }
  
  // Give system some breathing room
  delay(1);
}

/**
 * Handle specific states and transitions in the system
 * This is where you could implement system-level logic
 */
void handleCurrentState() {
  // Detect if all three coin counters are dispensing - this could indicate
  // a "make change" operation is in progress
  if (oneCounter.isDispensing() && fiveCounter.isDispensing() && tenCounter.isDispensing()) {
    // Special case handling could go here
  }
}

/**
 * Handle a command to dispense paper
 */
void handlePaperDispenseCommand(JsonDocument& doc) {
  if (doc.containsKey("value") && doc.containsKey("dispenser")) {
    int amount = doc["value"];
    int dispenser = doc["dispenser"];
    
    if (dispenser == 1) {
      paperDispenser1.dispense(amount);
    } else if (dispenser == 2) {
      paperDispenser2.dispense(amount);
    } else {
      // Default to first dispenser
      paperDispenser1.dispense(amount);
    }
  } else {
    // Missing parameters
    SerialComm::sendError("System", "dispense_paper", "missing_parameters");
  }
}

/**
 * Handle a command to dispense coins
 */
void handleCoinDispenseCommand(JsonDocument& doc) {
  if (doc.containsKey("value") && doc.containsKey("coin_type")) {
    int amount = doc["value"];
    int coinType = doc["coin_type"];
    
    switch (coinType) {
      case 1:
        oneCounter.dispense(amount);
        break;
      case 5:
        fiveCounter.dispense(amount);
        break;
      case 10:
        tenCounter.dispense(amount);
        break;
      default:
        // Invalid coin type
        SerialComm::sendError("System", "dispense_coin", "invalid_coin_type");
    }
  } else {
    // Missing parameters
    SerialComm::sendError("System", "dispense_coin", "missing_parameters");
  }
}
