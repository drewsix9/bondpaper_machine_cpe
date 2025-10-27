#include "StepperMotor.h"
#include "config.h"

// Create dispenser objects using config structures
PaperDispenser a4Dispenser(
    a4_step_motors.stepper.pulse_pin,
    a4_step_motors.stepper.dir_pin,
    a4_step_motors.dc_motor.IN1,
    a4_step_motors.dc_motor.IN2,
    a4_step_motors.dc_motor.en_pin,
    a4_step_motors.stepper.limit_switch,
    a4_step_motors.stepper.steps
);

PaperDispenser longDispenser(
    long_step_motors.stepper.pulse_pin,
    long_step_motors.stepper.dir_pin,
    long_step_motors.dc_motor.IN1,
    long_step_motors.dc_motor.IN2,
    long_step_motors.dc_motor.en_pin,
    long_step_motors.stepper.limit_switch,
    long_step_motors.stepper.steps
);

void setup() {
    Serial.begin(115200);
    Serial.println("Paper Dispenser System Starting...");
    Serial.println("=================================");
    
    // Initialize dispensers
    Serial.println("Initializing A4 dispenser...");
    a4Dispenser.begin();
    
    Serial.println("Initializing LONG dispenser...");
    longDispenser.begin();
    
    Serial.println("=================================");
    Serial.println("System ready!");
    Serial.println("");
    Serial.println("Available Commands:");
    Serial.println("  a4_dispense_X      - Dispense X papers from A4 (e.g., a4_dispense_5)");
    Serial.println("  long_dispense_X    - Dispense X papers from LONG (e.g., long_dispense_3)");
    Serial.println("  a4_steps_X         - Set A4 stepper steps to X (e.g., a4_steps_2000)");
    Serial.println("  long_steps_X       - Set LONG stepper steps to X (e.g., long_steps_2500)");
    Serial.println("  a4_ramp            - Ramp down A4 dispenser");
    Serial.println("  long_ramp          - Ramp down LONG dispenser");
    Serial.println("  a4_switch          - Check A4 limit switch status");
    Serial.println("  long_switch        - Check LONG limit switch status");
    Serial.println("  status             - Show system status");
    Serial.println("");
    Serial.println("Current Configuration:");
    Serial.print("  A4 Steps: "); Serial.println(a4_step_motors.stepper.steps);
    Serial.print("  LONG Steps: "); Serial.println(long_step_motors.stepper.steps);
    Serial.println("=================================");
}

void loop() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        Serial.print("Received command: ");
        Serial.println(cmd);
        
        // A4 dispenser commands
        if (cmd.startsWith("a4_dispense_")) {
            int papers = cmd.substring(12).toInt();
            if (papers > 0 && papers <= 100) {
                Serial.print("Starting A4 dispensing of ");
                Serial.print(papers);
                Serial.println(" papers...");
                a4Dispenser.dispense(papers);
            } else {
                Serial.println("Error: Invalid paper count (1-100)");
            }
        }
        
        // LONG dispenser commands
        else if (cmd.startsWith("long_dispense_")) {
            int papers = cmd.substring(14).toInt();
            if (papers > 0 && papers <= 100) {
                Serial.print("Starting LONG dispensing of ");
                Serial.print(papers);
                Serial.println(" papers...");
                longDispenser.dispense(papers);
            } else {
                Serial.println("Error: Invalid paper count (1-100)");
            }
        }
        
        // A4 stepper steps configuration
        else if (cmd.startsWith("a4_steps_")) {
            int steps = cmd.substring(9).toInt();
            if (steps > 0 && steps <= 10000) {
                a4Dispenser.setStepperSteps(steps);
            } else {
                Serial.println("Error: Invalid step count (1-10000)");
            }
        }
        
        // LONG stepper steps configuration
        else if (cmd.startsWith("long_steps_")) {
            int steps = cmd.substring(11).toInt();
            if (steps > 0 && steps <= 10000) {
                longDispenser.setStepperSteps(steps);
            } else {
                Serial.println("Error: Invalid step count (1-10000)");
            }
        }
        
        // Ramp down commands
        else if (cmd == "a4_ramp") {
            Serial.println("A4 ramp down initiated...");
            a4Dispenser.rampDown();
        }
        else if (cmd == "long_ramp") {
            Serial.println("LONG ramp down initiated...");
            longDispenser.rampDown();
        }
        
        // Limit switch status
        else if (cmd == "a4_switch") {
            Serial.print("A4 limit switch status: ");
            Serial.println(a4Dispenser.isLimitSwitchPressed() ? "PRESSED" : "NOT PRESSED");
        }
        else if (cmd == "long_switch") {
            Serial.print("LONG limit switch status: ");
            Serial.println(longDispenser.isLimitSwitchPressed() ? "PRESSED" : "NOT PRESSED");
        }
        
        // System status
        else if (cmd == "status") {
            printSystemStatus();
        }
        
        // Help command
        else if (cmd == "help") {
            printHelp();
        }
        
        // Unknown command
        else {
            Serial.println("Error: Unknown command. Type 'help' for available commands.");
        }
        
        Serial.println("Ready for next command...");
        Serial.println("");
    }
}

void printSystemStatus() {
    Serial.println("=== SYSTEM STATUS ===");
    Serial.println("A4 Dispenser:");
    Serial.print("  Limit Switch: ");
    Serial.println(a4Dispenser.isLimitSwitchPressed() ? "PRESSED" : "NOT PRESSED");
    Serial.print("  Pulse Pin: "); Serial.println(a4_step_motors.stepper.pulse_pin);
    Serial.print("  Dir Pin: "); Serial.println(a4_step_motors.stepper.dir_pin);
    Serial.print("  Limit Switch Pin: "); Serial.println(a4_step_motors.stepper.limit_switch);
    
    Serial.println("");
    Serial.println("LONG Dispenser:");
    Serial.print("  Limit Switch: ");
    Serial.println(longDispenser.isLimitSwitchPressed() ? "PRESSED" : "NOT PRESSED");
    Serial.print("  Pulse Pin: "); Serial.println(long_step_motors.stepper.pulse_pin);
    Serial.print("  Dir Pin: "); Serial.println(long_step_motors.stepper.dir_pin);
    Serial.print("  Limit Switch Pin: "); Serial.println(long_step_motors.stepper.limit_switch);
    
    Serial.println("");
    Serial.println("Pin Assignments:");
    Serial.print("  Buzzer: "); Serial.println(BUZZER_PIN);
    Serial.print("  Relay 1: "); Serial.println(RELAY1_PIN);
    Serial.print("  Relay 2: "); Serial.println(RELAY2_PIN);
    Serial.print("  Relay 3: "); Serial.println(RELAY3_PIN);
    Serial.println("=====================");
}

void printHelp() {
    Serial.println("=== HELP ===");
    Serial.println("Available Commands:");
    Serial.println("  a4_dispense_X      - Dispense X papers from A4");
    Serial.println("  long_dispense_X    - Dispense X papers from LONG");
    Serial.println("  a4_steps_X         - Set A4 stepper steps");
    Serial.println("  long_steps_X       - Set LONG stepper steps");
    Serial.println("  a4_ramp            - Ramp down A4 dispenser");
    Serial.println("  long_ramp          - Ramp down LONG dispenser");
    Serial.println("  a4_switch          - Check A4 limit switch");
    Serial.println("  long_switch        - Check LONG limit switch");
    Serial.println("  status             - Show system status");
    Serial.println("  help               - Show this help");
    Serial.println("=============");
}