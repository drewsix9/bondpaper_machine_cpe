#define RELAY1_PIN 4
#define RELAY2_PIN 5
#define RELAY3_PIN 6
#define COINS_PULSE_PIN 2
#define BUZZER_PIN 12

void setup() {
  Serial.begin(115200);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "relay1_on") {
      digitalWrite(RELAY1_PIN, LOW);
      Serial.println("Relay 1 ON");
    } else if (cmd == "relay1_off") {
      digitalWrite(RELAY1_PIN, HIGH);
      Serial.println("Relay 1 OFF");
    } else if (cmd == "relay2_on") {
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println("Relay 2 ON");
    } else if (cmd == "relay2_off") {
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println("Relay 2 OFF");
    } else if (cmd == "relay3_on") {
      digitalWrite(RELAY3_PIN, LOW);
      Serial.println("Relay 3 ON");
    } else if (cmd == "relay3_off") {
      digitalWrite(RELAY3_PIN, HIGH);
      Serial.println("Relay 3 OFF");
    } else {
      Serial.println("Unknown command");
    }
  }
}

// unsigned long previousMillis = 0;
// const long interval = 1000; // 1 second

// void loop(){
//   unsigned long currentMillis = millis();
//   if (currentMillis - previousMillis >= interval) {
//     previousMillis = currentMillis;
//     digitalWrite(RELAY1_PIN, !digitalRead(RELAY1_PIN));
//     digitalWrite(RELAY2_PIN, !digitalRead(RELAY2_PIN));
//     digitalWrite(RELAY3_PIN, !digitalRead(RELAY3_PIN));
//   }

// }