void setup() { Serial.begin(115200); }
void loop() {
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    Serial.println("ECHO:" + s);
  }
}