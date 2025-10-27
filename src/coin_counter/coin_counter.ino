#include "CoinCounter.h"
#include "RelayHopper.h"

#define RELAY1_PIN 4
#define RELAY2_PIN 5
#define RELAY3_PIN 6

#define ONE_SENSOR_PIN 8
#define FIVE_SENSOR_PIN 9
#define TEN_SENSOR_PIN 10

CoinCounter oneSensor(ONE_SENSOR_PIN, "oneSensor");
CoinCounter fiveSensor(FIVE_SENSOR_PIN, "fiveSensor");
CoinCounter tenSensor(TEN_SENSOR_PIN, "tenSensor");

RelayController relays(RELAY1_PIN, RELAY2_PIN, RELAY3_PIN);

void setup() {
    Serial.begin(115200);
    
    // Initialize coin counters
    // oneSensor.begin();
    // fiveSensor.begin();
    // tenSensor.begin();
    
    // Initialize relay controller
    relays.begin();

    Serial.println("Dual Coin Counter with Relay Control Ready");
    Serial.println("Timeout fixed to 3000ms (3 seconds)");
    Serial.println("Commands:");
    Serial.println("Coin Counter Commands:");
    Serial.println("  counter1_reset - Reset counter1");
    Serial.println("  counter1_get - Get counter1 value");
    Serial.println("  counter1_dispense_10 - Dispense 10 coins");
    Serial.println("  counter1_stop - Stop dispensing");
    Serial.println("  (Same commands for counter2 and counter3)");
    Serial.println("");
    Serial.println("Relay Commands:");
    Serial.println("  relay1_on/relay1_off - Control relay 1");
    Serial.println("  relay2_on/relay2_off - Control relay 2");
    Serial.println("  relay3_on/relay3_off - Control relay 3");
}

void loop() {
    // Update coin counters
    oneSensor.update();
    fiveSensor.update();
    tenSensor.update();


    // Centralized serial command handling
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        // Dispatch to coin counters
        if (cmd.startsWith("onesensor_")) {
            oneSensor.handleSerial(cmd);
        } else if (cmd.startsWith("fivesensor_")) {
            fiveSensor.handleSerial(cmd);
        } else if (cmd.startsWith("tensensor_")) {
            tenSensor.handleSerial(cmd);
        } else if (cmd.startsWith("relay")) {
            relays.handleSerial(cmd);
        } else {
            Serial.println("Unknown command: " + cmd);
        }
      }


    // Check for dispensing completion and control relays accordingly
    if (oneSensor.hasReachedTarget()) {
        Serial.println("oneSensor dispensing complete - target reached!");
        relays.setRelay(1, false); // Turn off relay 1
    }

    if (oneSensor.hasTimedOut()) {
        Serial.println("oneSensor dispensing complete - timeout!");
        relays.setRelay(1, false); // Turn off relay 1
    }

    if (fiveSensor.hasReachedTarget()) {
        Serial.println("fiveSensor dispensing complete - target reached!");
        relays.setRelay(2, false); // Turn off relay 2
    }

    if (fiveSensor.hasTimedOut()) {
        Serial.println("fiveSensor dispensing complete - timeout!");
        relays.setRelay(2, false); // Turn off relay 2
    }

    if (tenSensor.hasReachedTarget()) {
        Serial.println("tenSensor dispensing complete - target reached!");
        relays.setRelay(3, false); // Turn off relay 3
    }

    if (tenSensor.hasTimedOut()) {
        Serial.println("tenSensor dispensing complete - timeout!");
        relays.setRelay(3, false); // Turn off relay 3
    }
}