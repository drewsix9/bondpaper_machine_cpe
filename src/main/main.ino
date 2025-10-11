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

// JSON document for system status and info
StaticJsonDocument<512> systemInfoDoc;

// ========== TIMING VARIABLES ==========
unsigned long lastStatusTime = 0;
const unsigned long STATUS_INTERVAL = 5000;  // Status update every 5 seconds

// ========== SYSTEM FUNCTIONS ==========

/**
 * Send a system-level acknowledgement for commands that 
 * aren't handled by specific modules
 */
void sendSystemAck(const char* cmd, bool ok, const char* status = nullptr) {
  systemInfoDoc.clear();
  
  systemInfoDoc["v"] = 1;
  systemInfoDoc["source"] = "System";
  systemInfoDoc["type"] = "ack";
  systemInfoDoc["ts"] = millis();
  
  JsonObject data = systemInfoDoc.createNestedObject("data");
  data["cmd"] = cmd;
  data["ok"] = ok;
  
  if (status) {
    data["status"] = status;
  }
  
  serializeJson(systemInfoDoc, Serial);
  Serial.println();
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
  
  serializeJson(systemInfoDoc, Serial);
  Serial.println();
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
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, line);
  
  if (!error && doc.containsKey("v") && doc.containsKey("target") && doc.containsKey("cmd")) {
    // Valid JSON command - route to appropriate module
    const char* target = doc["target"];
    
    if (strcmp(target, "CoinSlot") == 0) {
      coinSlot.handleSerial(line);
    }
    else if (strcmp(target, "PaperDispenser") == 0) {
      // Check name to route to correct dispenser
      if (doc.containsKey("name")) {
        const char* name = doc["name"];
        if (strcmp(name, "a4Dispenser") == 0) {
          paperDispenser1.handleSerial(line);
        } 
        else if (strcmp(name, "longDispenser") == 0) {
          paperDispenser2.handleSerial(line);
        }
      } else {
        // Default to paper dispenser 1
        paperDispenser1.handleSerial(line);
      }
    }
    else if (strcmp(target, "CoinCounter") == 0) {
      // All coin counters check name in their handleSerial
      oneCounter.handleSerial(line);
      fiveCounter.handleSerial(line);
      tenCounter.handleSerial(line);
    }
    else if (strcmp(target, "Relay") == 0) {
      relayController.handleSerial(line);
    }
    else if (strcmp(target, "System") == 0) {
      processSystemCommand(doc);
    }
    else {
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
    coinSlot.handleSerial(line);
    paperDispenser1.handleSerial(line);
    paperDispenser2.handleSerial(line);
    oneCounter.handleSerial(line);
    fiveCounter.handleSerial(line);
    tenCounter.handleSerial(line);
    relayController.handleSerial(line);
  }
}

/**
 * Print current system status to Serial (for debugging)
 */
void printSystemStatus() {
  Serial.println(F("===== SYSTEM STATUS ====="));
  
  // Limit switch status
  Serial.print(F("Paper1 Limit: "));
  Serial.println(paperDispenser1.isLimitSwitchPressed() ? F("PRESSED") : F("RELEASED"));
  Serial.print(F("Paper2 Limit: "));
  Serial.println(paperDispenser2.isLimitSwitchPressed() ? F("PRESSED") : F("RELEASED"));

  // Coin Counter status
  Serial.print(F("Peso1: "));
  Serial.print(oneCounter.getCount());
  Serial.print(F(" - "));
  Serial.println(oneCounter.isDispensing() ? F("DISPENSING") : F("IDLE"));
  
  Serial.print(F("Peso5: "));
  Serial.print(fiveCounter.getCount());
  Serial.print(F(" - "));
  Serial.println(fiveCounter.isDispensing() ? F("DISPENSING") : F("IDLE"));
  
  Serial.print(F("Peso10: "));
  Serial.print(tenCounter.getCount());
  Serial.print(F(" - "));
  Serial.println(tenCounter.isDispensing() ? F("DISPENSING") : F("IDLE"));
  
  Serial.println(F("========================="));
}

// ========== MAIN ARDUINO FUNCTIONS ==========

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  delay(100);
  
  Serial.println(F("{\"v\":1,\"source\":\"System\",\"type\":\"event\",\"ts\":0,\"data\":{\"event\":\"boot\"}}"));
  
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
  
  // Periodically send system status during activity
  // Only if any subsystem is active to avoid flooding the serial
  bool systemActive = coinSlot.isAttached() || 
                      paperDispenser1.isDispensing() || 
                      paperDispenser2.isDispensing() || 
                      oneCounter.isDispensing() ||
                      fiveCounter.isDispensing() ||
                      tenCounter.isDispensing();
                      
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
    systemInfoDoc.clear();
    systemInfoDoc["v"] = 1;
    systemInfoDoc["source"] = "System";
    systemInfoDoc["type"] = "error";
    systemInfoDoc["ts"] = millis();
    
    JsonObject data = systemInfoDoc.createNestedObject("data");
    data["action"] = "dispense_paper";
    data["error"] = "missing_parameters";
    
    serializeJson(systemInfoDoc, Serial);
    Serial.println();
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
        systemInfoDoc.clear();
        systemInfoDoc["v"] = 1;
        systemInfoDoc["source"] = "System";
        systemInfoDoc["type"] = "error";
        systemInfoDoc["ts"] = millis();
        
        JsonObject data = systemInfoDoc.createNestedObject("data");
        data["action"] = "dispense_coin";
        data["error"] = "invalid_coin_type";
        
        serializeJson(systemInfoDoc, Serial);
        Serial.println();
    }
  } else {
    // Missing parameters
    systemInfoDoc.clear();
    systemInfoDoc["v"] = 1;
    systemInfoDoc["source"] = "System";
    systemInfoDoc["type"] = "error";
    systemInfoDoc["ts"] = millis();
    
    JsonObject data = systemInfoDoc.createNestedObject("data");
    data["action"] = "dispense_coin";
    data["error"] = "missing_parameters";
    
    serializeJson(systemInfoDoc, Serial);
    Serial.println();
  }
}
