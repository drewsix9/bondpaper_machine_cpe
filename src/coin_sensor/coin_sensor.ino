#include "coin_sensor.h"

#define ONE_SENSOR_PIN 8
#define FIVE_SENSOR_PIN 9
#define TEN_SENSOR_PIN 10

CoinSensor oneSensor (ONE_SENSOR_PIN, 0, 1);
CoinSensor fiveSensor (FIVE_SENSOR_PIN, 0, 5);
CoinSensor tenSensor (TEN_SENSOR_PIN, 0, 10);

void setup() {
  Serial.begin(115000);
  oneSensor.begin();
  fiveSensor.begin();
  tenSensor.begin();
}

void loop() {
  oneSensor.handleSerial();
  fiveSensor.handleSerial();
  tenSensor.handleSerial();
}