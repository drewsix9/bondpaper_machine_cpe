// New paper-present limit switches (separate use-case)
#define LIMIT_SHORT_PRES   33
#define LIMIT_LONG_PRES    34

void setup() {
  Serial.println("Paper Present Test");
  pinMode(LIMIT_SHORT_PRES, INPUT_PULLUP);
  pinMode(LIMIT_LONG_PRES,  INPUT_PULLUP);
  Serial.begin(115200);
}

inline bool paperPresent(int pin)     { return digitalRead(pin) == LOW;  } // pressed
inline bool noPaperOrFault(int pin)   { return digitalRead(pin) == HIGH; } // open or broken

void loop() {
  if (noPaperOrFault(LIMIT_SHORT_PRES)) {
    Serial.println("[SHORT] NO PAPER / SENSOR OPEN");
    // stop short feed motor, raise alarm, etc.
  } else {
    Serial.println("[SHORT] Paper present");
  }

  if (noPaperOrFault(LIMIT_LONG_PRES)) {
    Serial.println("[LONG] NO PAPER / SENSOR OPEN");
    // stop long feed motor, raise alarm, etc.
  } else {
    Serial.println("[LONG] Paper present");
  }

  delay(50);
}
