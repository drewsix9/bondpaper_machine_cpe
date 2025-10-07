// Motor system structure
struct StepperConfig {
  uint8_t dir_pin;
  uint8_t pulse_pin;
  int steps;
};

// Configuration - only stepper motor pins
static const StepperConfig a4_stepper = {
  .dir_pin = 22,        // Digital pin 22
  .pulse_pin = 23,      // Digital pin 23
  .steps = 1900
};
  
static const StepperConfig long_stepper = {
  .dir_pin = 28,        // Digital pin 28
  .pulse_pin = 29,      // Digital pin 29
  .steps = 2200
};

// Direction constants
#define CW  1
#define CCW 0

// Simple Stepper Motor Class
class SimpleStepper {
private:
  uint8_t dirPin;
  uint8_t pulsePin;
  int defaultSteps;
  unsigned int stepDelay;

public:
  SimpleStepper(uint8_t dir, uint8_t pulse, int steps) {
    dirPin = dir;
    pulsePin = pulse;
    defaultSteps = steps;
    stepDelay = 1000; // 1ms default delay
  }

  void begin() {
    pinMode(dirPin, OUTPUT);
    pinMode(pulsePin, OUTPUT);
    
    digitalWrite(dirPin, LOW);
    digitalWrite(pulsePin, LOW);
    
    Serial.print("Stepper initialized - Dir: ");
    Serial.print(dirPin);
    Serial.print(", Pulse: ");
    Serial.print(pulsePin);
    Serial.print(", Default Steps: ");
    Serial.println(defaultSteps);
  }

  void rotate(int steps, uint8_t direction, unsigned int delayMicros = 1000) {
    Serial.print("Rotating ");
    Serial.print(steps);
    Serial.print(" steps ");
    Serial.println(direction == CW ? "CW" : "CCW");
    
    // Set direction
    digitalWrite(dirPin, direction == CW ? HIGH : LOW);
    delay(1); // Small delay for direction change
    
    // Perform steps
    for (int i = 0; i < abs(steps); i++) {
      digitalWrite(pulsePin, HIGH);
      delayMicroseconds(delayMicros);
      digitalWrite(pulsePin, LOW);
      delayMicroseconds(delayMicros);
      
      // Progress indicator every 200 steps
      if ((i + 1) % 200 == 0) {
        Serial.print("Step ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.println(abs(steps));
      }
    }
    
    Serial.println("Rotation complete");
  }

  void rotateDefault(uint8_t direction) {
    rotate(defaultSteps, direction, stepDelay);
  }

  void setSpeed(unsigned int delayMicros) {
    stepDelay = delayMicros;
    Serial.print("Speed set to ");
    Serial.print(delayMicros);
    Serial.println(" microseconds delay");
  }

  int getDefaultSteps() {
    return defaultSteps;
  }

  void setDefaultSteps(int steps) {
    defaultSteps = steps;
    Serial.print("Default steps set to ");
    Serial.println(steps);
  }
};

// Create stepper motor instances
SimpleStepper a4Motor(a4_stepper.dir_pin, a4_stepper.pulse_pin, a4_stepper.steps);
SimpleStepper longMotor(long_stepper.dir_pin, long_stepper.pulse_pin, long_stepper.steps);

void setup() {
  Serial.begin(115200);
  Serial.println("=== STEPPER MOTOR ONLY TEST ===");
  Serial.println("Initializing stepper motors...");
  
  a4Motor.begin();
  longMotor.begin();
  
  Serial.println("\nSystem ready!");
  printHelp();
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    Serial.print("Command: ");
    Serial.println(cmd);
    
    handleCommand(cmd);
    
    Serial.println("Ready for next command...\n");
  }
}

void handleCommand(String cmd) {
  // A4 Stepper Commands
  if (cmd == "a4_cw") {
    a4Motor.rotateDefault(CW);
  }
  else if (cmd == "a4_ccw") {
    a4Motor.rotateDefault(CCW);
  }
  else if (cmd.startsWith("a4_cw_")) {
    int steps = cmd.substring(6).toInt();
    if (steps > 0 && steps <= 10000) {
      a4Motor.rotate(steps, CW);
    } else {
      Serial.println("Error: Steps must be 1-10000");
    }
  }
  else if (cmd.startsWith("a4_ccw_")) {
    int steps = cmd.substring(7).toInt();
    if (steps > 0 && steps <= 10000) {
      a4Motor.rotate(steps, CCW);
    } else {
      Serial.println("Error: Steps must be 1-10000");
    }
  }
  else if (cmd.startsWith("a4_speed_")) {
    int speed = cmd.substring(9).toInt();
    if (speed >= 100 && speed <= 10000) {
      a4Motor.setSpeed(speed);
    } else {
      Serial.println("Error: Speed must be 100-10000 microseconds");
    }
  }
  else if (cmd.startsWith("a4_steps_")) {
    int steps = cmd.substring(9).toInt();
    if (steps > 0 && steps <= 10000) {
      a4Motor.setDefaultSteps(steps);
    } else {
      Serial.println("Error: Steps must be 1-10000");
    }
  }
  
  // LONG Stepper Commands
  else if (cmd == "long_cw") {
    longMotor.rotateDefault(CW);
  }
  else if (cmd == "long_ccw") {
    longMotor.rotateDefault(CCW);
  }
  else if (cmd.startsWith("long_cw_")) {
    int steps = cmd.substring(9).toInt();
    if (steps > 0 && steps <= 10000) {
      longMotor.rotate(steps, CW);
    } else {
      Serial.println("Error: Steps must be 1-10000");
    }
  }
  else if (cmd.startsWith("long_ccw_")) {
    int steps = cmd.substring(10).toInt();
    if (steps > 0 && steps <= 10000) {
      longMotor.rotate(steps, CCW);
    } else {
      Serial.println("Error: Steps must be 1-10000");
    }
  }
  else if (cmd.startsWith("long_speed_")) {
    int speed = cmd.substring(11).toInt();
    if (speed >= 100 && speed <= 10000) {
      longMotor.setSpeed(speed);
    } else {
      Serial.println("Error: Speed must be 100-10000 microseconds");
    }
  }
  else if (cmd.startsWith("long_steps_")) {
    int steps = cmd.substring(11).toInt();
    if (steps > 0 && steps <= 10000) {
      longMotor.setDefaultSteps(steps);
    } else {
      Serial.println("Error: Steps must be 1-10000");
    }
  }
  
  // System Commands
  else if (cmd == "status") {
    printStatus();
  }
  else if (cmd == "help") {
    printHelp();
  }
  
  // Test sequences
  else if (cmd == "test_a4") {
    testA4();
  }
  else if (cmd == "test_long") {
    testLong();
  }
  else if (cmd == "test_both") {
    testBoth();
  }
  
  else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

void printHelp() {
  Serial.println("\n=== AVAILABLE COMMANDS ===");
  Serial.println("A4 Stepper Commands:");
  Serial.println("  a4_cw                - Rotate CW default steps (1900)");
  Serial.println("  a4_ccw               - Rotate CCW default steps (1900)");
  Serial.println("  a4_cw_X              - Rotate CW X steps (1-10000)");
  Serial.println("  a4_ccw_X             - Rotate CCW X steps (1-10000)");
  Serial.println("  a4_speed_X           - Set speed delay X μs (100-10000)");
  Serial.println("  a4_steps_X           - Set default steps to X (1-10000)");
  Serial.println("");
  Serial.println("LONG Stepper Commands:");
  Serial.println("  long_cw              - Rotate CW default steps (2200)");
  Serial.println("  long_ccw             - Rotate CCW default steps (2200)");
  Serial.println("  long_cw_X            - Rotate CW X steps (1-10000)");
  Serial.println("  long_ccw_X           - Rotate CCW X steps (1-10000)");
  Serial.println("  long_speed_X         - Set speed delay X μs (100-10000)");
  Serial.println("  long_steps_X         - Set default steps to X (1-10000)");
  Serial.println("");
  Serial.println("System Commands:");
  Serial.println("  status               - Show system status");
  Serial.println("  test_a4              - Run A4 test sequence");
  Serial.println("  test_long            - Run LONG test sequence");
  Serial.println("  test_both            - Run both test sequences");
  Serial.println("  help                 - Show this help");
  Serial.println("==========================\n");
}

void printStatus() {
  Serial.println("\n=== SYSTEM STATUS ===");
  Serial.print("A4 Stepper (Pins ");
  Serial.print(a4_stepper.dir_pin);
  Serial.print("/");
  Serial.print(a4_stepper.pulse_pin);
  Serial.print(") - Default Steps: ");
  Serial.println(a4Motor.getDefaultSteps());
  
  Serial.print("LONG Stepper (Pins ");
  Serial.print(long_stepper.dir_pin);
  Serial.print("/");
  Serial.print(long_stepper.pulse_pin);
  Serial.print(") - Default Steps: ");
  Serial.println(longMotor.getDefaultSteps());
  Serial.println("=====================\n");
}

void testA4() {
  Serial.println("=== A4 STEPPER TEST ===");
  Serial.println("Test 1: CW 500 steps");
  a4Motor.rotate(500, CW, 2000);
  delay(1000);
  
  Serial.println("Test 2: CCW 500 steps");
  a4Motor.rotate(500, CCW, 2000);
  delay(1000);
  
  Serial.println("Test 3: Fast CW 1000 steps");
  a4Motor.rotate(1000, CW, 500);
  delay(1000);
  
  Serial.println("A4 test complete!");
}

void testLong() {
  Serial.println("=== LONG STEPPER TEST ===");
  Serial.println("Test 1: CW 500 steps");
  longMotor.rotate(500, CW, 2000);
  delay(1000);
  
  Serial.println("Test 2: CCW 500 steps");
  longMotor.rotate(500, CCW, 2000);
  delay(1000);
  
  Serial.println("Test 3: Fast CW 1000 steps");
  longMotor.rotate(1000, CW, 500);
  delay(1000);
  
  Serial.println("LONG test complete!");
}

void testBoth() {
  Serial.println("=== TESTING BOTH STEPPERS ===");
  
  Serial.println("Phase 1: Both CW 200 steps");
  a4Motor.rotate(1900, CW, 1500);
  longMotor.rotate(2200, CW, 1500);
  delay(1000);
  
  Serial.println("Phase 2: Both CCW 200 steps");
  a4Motor.rotate(1900, CCW, 1500);
  longMotor.rotate(2200, CCW, 1500);
  delay(1000);
  
  Serial.println("Phase 3: Alternating");
  a4Motor.rotate(1900, CW, 1000);
  delay(500);
  longMotor.rotate(2200, CW, 1000);
  delay(500);
  
  Serial.println("Both motors test complete!");
}