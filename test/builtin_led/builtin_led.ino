#define BUZZER_PIN 12

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("Mega online");
  pinMode(LED_BUILTIN, OUTPUT); // Initialize built-in LED
  pinMode(BUZZER_PIN, OUTPUT);  // Initialize buzzer
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "on") {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("LED ON");
      tone(BUZZER_PIN, 2000); // Turn on buzzer at 1kHz
    } else if (cmd == "off") {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("LED OFF");
      noTone(BUZZER_PIN); // Turn off buzzer
    }
  }
}
