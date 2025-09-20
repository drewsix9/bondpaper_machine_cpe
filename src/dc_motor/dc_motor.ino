#include "config.h"
#include "dc_motor.h"

  DCMotor a4Motor(A4_IN1, A4_IN2, A4_EN, "a4");
  DCMotor longMotor(LONG_IN1, LONG_IN2, LONG_EN, "long");

void setup() {
  Serial.begin(115200);
  a4Motor.begin();
  longMotor.begin();
  Serial.println("DC Motor Control Ready");
}

void loop() {
  a4Motor.handleSerial();
  longMotor.handleSerial();
}
