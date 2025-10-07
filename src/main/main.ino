#include "config.h"         // pins & motor configs (A4/LONG, relays, sensors)  // :contentReference[oaicite:5]{index=5}
#include "CoinSlotISR.h"    // pulse-based coin insert ISR                      // :contentReference[oaicite:6]{index=6}
#include "CoinCounter.h"    // per-hopper coin-out counters with timeout        // :contentReference[oaicite:7]{index=7}
#include "RelayHopper.h"    // 3-ch relay controller (active-low)               // :contentReference[oaicite:8]{index=8}
#include "StepperMotor.h"   // PaperDispenser (stepper + dc + limit switch)     // :contentReference[oaicite:9]{index=9}

/* ---------------- Instances ---------------- */

CoinSlotISR coinSlot(COINS_PULSE_PIN);

// Map: Relay1 -> 1 peso, Relay2 -> 5 peso, Relay3 -> 10 peso (adjust if different)
RelayController relays(RELAY1_PIN, RELAY2_PIN, RELAY3_PIN);

// Coin-out sensors for each hopper (falling edge -> ezButton inside CoinCounter)
CoinCounter oneCounter(ONE_SENSOR_PIN,  String("ONE"));
CoinCounter fiveCounter(FIVE_SENSOR_PIN, String("FIVE"));
CoinCounter tenCounter(TEN_SENSOR_PIN,   String("TEN"));

// Paper dispensers (A4 & LONG) wired from config.h (dir, pulse, limit, steps; IN1, IN2, EN)
PaperDispenser dispA4(
  a4_step_motors.stepper.pulse_pin,
  a4_step_motors.stepper.dir_pin,
  a4_step_motors.dc_motor.IN1,
  a4_step_motors.dc_motor.IN2,
  a4_step_motors.dc_motor.en_pin,
  a4_step_motors.stepper.limit_switch,
  a4_step_motors.stepper.steps
);
PaperDispenser dispLONG(
  long_step_motors.stepper.pulse_pin,
  long_step_motors.stepper.dir_pin,
  long_step_motors.dc_motor.IN1,
  long_step_motors.dc_motor.IN2,
  long_step_motors.dc_motor.en_pin,
  long_step_motors.stepper.limit_switch,
  long_step_motors.stepper.steps
);

/* ---------------- Helpers ---------------- */

static inline void relayOnForValue(int value) {
  // NOTE: RelayController::setRelay(relayNum, false) prints "ON" (active-low).  // :contentReference[oaicite:10]{index=10}
  if (value == 1)      relays.setRelay(1, false);
  else if (value == 5) relays.setRelay(2, false);
  else if (value == 10)relays.setRelay(3, false);
}

static inline void relayOffForValue(int value) {
  if (value == 1)      relays.setRelay(1, true);
  else if (value == 5) relays.setRelay(2, true);
  else if (value == 10)relays.setRelay(3, true);
}

static CoinCounter* counterForValue(int value) {
  if (value == 1)  return &oneCounter;
  if (value == 5)  return &fiveCounter;
  if (value == 10) return &tenCounter;
  return nullptr;
}

// Synchronous coin-dispense loop: runs until target reached or timeout
bool dispenseCoinValue(int value, int count, unsigned long hardStopMs = 20000UL) {
  CoinCounter* c = counterForValue(value);
  if (!c || count <= 0) {
    Serial.println(F("{\"ok\":false,\"error\":\"bad_value_or_count\"}"));
    return false;
  }

  // Prepare counter state & power the hopper relay
  c->dispense(count);          // sets target & starts timeout window           // :contentReference[oaicite:11]{index=11}
  relayOnForValue(value);

  unsigned long t0 = millis();
  while (true) {
    // update counters frequently to catch pulses and check timeout
    oneCounter.update();
    fiveCounter.update();
    tenCounter.update();

    if (c->hasReachedTarget()) {
      relayOffForValue(value);
      Serial.print(F("{\"ok\":true,\"cmd\":\"dispense_coin\",\"value\":"));
      Serial.print(value);
      Serial.print(F(",\"dispensed\":"));
      Serial.print(count);
      Serial.println(F("}"));
      return true;
    }
    if (c->hasTimedOut() || (millis() - t0 > hardStopMs)) {
      relayOffForValue(value);
      Serial.print(F("{\"ok\":false,\"error\":\"coin_timeout\",\"value\":"));
      Serial.print(value);
      Serial.print(F(",\"counted\":"));
      Serial.print(c->getCount());
      Serial.println(F("}"));
      return false;
    }
    // breathe
    delay(2);
  }
}

// Greedy plan using 10,5,1 (change this if you add 20P hardware)
void dispenseAmountPlan(int amount) {
  if (amount <= 0) {
    Serial.println(F("{\"ok\":true,\"cmd\":\"dispense_amount\",\"dispensed\":0}"));
    return;
  }
  int plan10 = amount / 10; amount %= 10;
  int plan5  = amount / 5;  amount %= 5;
  int plan1  = amount;

  bool ok = true;
  if (plan10) ok &= dispenseCoinValue(10, plan10);
  if (plan5)  ok &= dispenseCoinValue(5,  plan5);
  if (plan1)  ok &= dispenseCoinValue(1,  plan1);

  Serial.print(F("{\"ok\":"));
  Serial.print(ok ? F("true") : F("false"));
  Serial.print(F(",\"cmd\":\"dispense_amount\",\"p10\":"));
  Serial.print(plan10);
  Serial.print(F(",\"p5\":"));
  Serial.print(plan5);
  Serial.print(F(",\"p1\":"));
  Serial.print(plan1);
  Serial.println(F("}"));
}

void startCoinSlotISR() {
  coinSlot.begin();  // sets pin & attachInterrupt                         // :contentReference[oaicite:12]{index=12}
  Serial.println(F("{\"ok\":true,\"event\":\"coinslot_started\"}"));
  Serial.flush();
}

void stopCoinSlotISR() {
  detachInterrupt(digitalPinToInterrupt(COINS_PULSE_PIN));
  Serial.println(F("{\"ok\":true,\"event\":\"coinslot_stopped\"}"));
}

void resetSystem() {
  // Stop all relays
  relays.setRelay(1, true);
  relays.setRelay(2, true);
  relays.setRelay(3, true);

  // Reset counters
  oneCounter.reset();
  fiveCounter.reset();
  tenCounter.reset();

  // Reset coin-in total via CoinSlot parser (uses its own command API)
  coinSlot.processCommand(String("reset"));                                      // :contentReference[oaicite:13]{index=13}

  Serial.println(F("{\"ok\":true,\"event\":\"system_reset\"}"));
}

/* ---------------- Setup / Loop ---------------- */

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for USB */ }

  relays.begin();                // initialize 3 relays (defaults: OFF/HIGH)     // :contentReference[oaicite:14]{index=14}
  oneCounter.begin();
  fiveCounter.begin();
  tenCounter.begin();
  dispA4.begin();
  dispLONG.begin();

  // Don’t start coin ISR by default; wait for 'coinslot_start'
  Serial.println(F("{\"event\":\"boot\",\"ready\":true}"));
}

String line;

void loop() {
  // Keep coin-in edge aggregator alive even when not inserting
  coinSlot.handle();  // converts pulse bursts -> denomination + totals        // :contentReference[oaicite:15]{index=15}

  // Keep coin-out counters updated
  oneCounter.update();
  fiveCounter.update();
  tenCounter.update();

  // Read a line from Serial
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n') {
      line.trim();
      if (line.length()) handleCommand(line);
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
}

/* ---------------- Command parser ----------------

Supported commands (space-separated):

  coinslot_start
  coinslot_stop
  paper A4 <n>
  paper LONG <n>
  dispense_coin <value> <count>    // value ∈ {1,5,10}
  dispense_amount <amount>         // uses 10/5/1 greedy
  get                               // forwarded to CoinSlot (reports inserted total)
  reset                             // resets counters/relays/coin-in total

-------------------------------------------------- */

void handleCommand(const String& cmd) {
  if (cmd == "coinslot_start") { startCoinSlotISR(); return; }
  if (cmd == "coinslot_stop")  { stopCoinSlotISR();  return; }
  if (cmd == "reset")          { resetSystem();      return; }
  if (cmd == "get")            { coinSlot.processCommand(String("get")); return; }  // Pi compatibility

  if (cmd.startsWith("paper ")) {
    // "paper A4 3" or "paper LONG 2"
    String rest = cmd.substring(6);
    rest.trim();
    int sp = rest.indexOf(' ');
    String kind = (sp == -1) ? rest : rest.substring(0, sp);
    int qty = (sp == -1) ? 1 : rest.substring(sp + 1).toInt();
    qty = max(1, qty);

    if (kind.equalsIgnoreCase("A4")) {
      dispA4.dispense(qty);      // blocking until complete                   // :contentReference[oaicite:16]{index=16}
      Serial.println(F("{\"ok\":true,\"cmd\":\"paper\",\"type\":\"A4\"}"));
    } else if (kind.equalsIgnoreCase("LONG")) {
      dispLONG.dispense(qty);
      Serial.println(F("{\"ok\":true,\"cmd\":\"paper\",\"type\":\"LONG\"}"));
    } else {
      Serial.println(F("{\"ok\":false,\"error\":\"unknown_paper\"}"));
    }
    return;
  }

  if (cmd.startsWith("dispense_coin ")) {
    // "dispense_coin 5 2"
    String rest = cmd.substring(String("dispense_coin ").length());
    rest.trim();
    int sp = rest.indexOf(' ');
    int value = (sp == -1) ? rest.toInt() : rest.substring(0, sp).toInt();
    int count = (sp == -1) ? 1 : rest.substring(sp + 1).toInt();
    if (value == 1 || value == 5 || value == 10) {
      dispenseCoinValue(value, max(1, count));
    } else {
      Serial.println(F("{\"ok\":false,\"error\":\"unsupported_value\"}"));
    }
    return;
  }

  if (cmd.startsWith("dispense_amount ")) {
    int amount = cmd.substring(String("dispense_amount ").length()).toInt();
    dispenseAmountPlan(max(0, amount));
    return;
  }

  Serial.println(F("{\"ok\":false,\"error\":\"unknown_command\"}"));
}
