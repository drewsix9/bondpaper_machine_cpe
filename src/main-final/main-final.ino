#include <Arduino.h>

// Modules
#include "config.h"
#include "InsertMeter.h"
#include "HopperPayout.h"
#include "PaperDispenser.h"

// ---------- Globals ----------

// Coin acceptor (coins INSERTED)
InsertMeter acceptor;

// ISR thunk for coin acceptor
void ISR_acceptor() { acceptor.isr(); }

// Hoppers (coins DISPENSED / change return)
HopperPayout hop1, hop5, hop10;

// Paper (SHORT, LONG)
PaperDispenser dispShort, dispLong;

// Actuator polarity for your relays/MOSFETs
static constexpr bool ACT_ACTIVE_LOW = true;

// ------------ CHANGE state machine ------------
struct ChangeState {
  bool active = false;
  uint16_t amt = 0;
  uint16_t t10 = 0, t5 = 0, t1 = 0;
  uint8_t stage = 0;   // 0 = idle, 10 = paying 10s, 5 = paying 5s, 1 = paying 1s
} change;

// ---------- Small helpers ----------

static String readLine() {
  static String buf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { String s = buf; buf = ""; if (s.length()) return s; }
    else { buf += c; if (buf.length() > 96) buf = ""; }
  }
  return String();
}

void start_change(uint16_t amount) {
  
  // Reset all hoppers before starting
  hop10.reset();
  delay(100);
  hop5.reset();
  delay(100);
  hop1.reset();
  delay(100);
  
  change.active = true;
  change.amt = amount;
  change.t10 = amount / 10; amount -= change.t10 * 10;
  change.t5  = amount / 5;  amount -= change.t5  * 5;
  change.t1  = amount;
  change.stage = 10;

  // Kick first needed hopper immediately with a short delay before starting
  delay(200);
  if      (change.t10) { hop10.start(change.t10); }
  else if (change.t5)  { change.stage = 5;  hop5.start(change.t5); }
  else if (change.t1)  { change.stage = 1;  hop1.start(change.t1); }
  else { // nothing to pay
    change.active = false; change.stage = 0;
    Serial.print("DONE CHANGE "); Serial.println(change.amt);
  }
}

void stop_all_change() {
  // Forcibly stop all hoppers
  hop10.stop();
  delay(50);
  hop5.stop(); 
  delay(50);
  hop1.stop();
  delay(50);
  
  // Now do a full reset of each hopper to ensure clean state
  hop10.reset();
  delay(100);
  hop5.reset();
  delay(100);
  hop1.reset();
  delay(100);
  
  // Completely reset change state to initial values
  change = ChangeState{}; // reset
  
  // Give the system a final moment to settle
  delay(200);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  // --- Coin acceptor input & ISR ---
  pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_COIN_PULSE), ISR_acceptor, FALLING);
  // We call acceptor.loop() in loop() to classify bursts into values per Config.h.

  // --- Hoppers: map Config.h pins to HopperPayouts ---
  HopperPins p1  { PIN_HOPPER_P1,  PIN_HOPPER_P1_SENS,  ACT_ACTIVE_LOW };
  HopperPins p5  { PIN_HOPPER_P5,  PIN_HOPPER_P5_SENS,  ACT_ACTIVE_LOW };
  HopperPins p10 { PIN_HOPPER_P10, PIN_HOPPER_P10_SENS, ACT_ACTIVE_LOW };
  hop1.begin (1,  p1);    // emits "OUT 1", "DONE 1 N", "ERR TIMEOUT 1 x/y"
  hop5.begin (5,  p5);
  hop10.begin(10, p10);

  // --- Paper: SHORT module (DIR/PULSE/limit + DC driver) ---
  dispShort.begin(
    SHORT_DC_IN1, SHORT_DC_IN2, SHORT_DC_EN,
    SHORT_LIMIT_SWITCH,
    SHORT_PIN_PULSE, SHORT_PIN_DIR, SHORT_STEP_US,
    SHORT_STEPS_PER_SHEET
  );

  // --- Paper: LONG module ---
  dispLong.begin(
    LONG_DC_IN1, LONG_DC_IN2, LONG_DC_EN,
    LONG_LIMIT_SWITCH,
    LONG_PIN_PULSE, LONG_PIN_DIR, LONG_STEP_US,
    LONG_STEPS_PER_SHEET
  );
}

// ---------- Loop ----------
// Variables for tracking change activity and timeout detection
static uint32_t lastChangeActivity = 0;
static const uint32_t CHANGE_TIMEOUT_MS = 30000; // 30 seconds timeout for change operations

void loop() {
  // Service modules
  acceptor.loop();          // classifies pulse bursts into 1/5/10/20 values (INSERTED)
  hop10.loop(); hop5.loop(); hop1.loop();

  // Handle CHANGE progression when each hopper completes (DONE/ERR prints are produced by hopper)
  if (change.active) {
    // Record activity when processing change
    lastChangeActivity = millis();
    
    if (change.stage == 10 && !hop10.busy()) {
      change.stage = 5;
      // Add a delay before starting the next hopper
      delay(500);  // 500ms delay between hoppers
      if      (change.t5) { hop5.start(change.t5); }
      else if (change.t1) { change.stage = 1; delay(500); hop1.start(change.t1); }
      else { change.active = false; change.stage = 0; Serial.print("DONE CHANGE "); Serial.println(change.amt); }
    } else if (change.stage == 5 && !hop5.busy()) {
      change.stage = 1;
      // Add a delay before starting the next hopper
      delay(500);  // 500ms delay between hoppers
      if      (change.t1) { hop1.start(change.t1); }
      else { change.active = false; change.stage = 0; Serial.print("DONE CHANGE "); Serial.println(change.amt); }
    } else if (change.stage == 1 && !hop1.busy()) {
      change.active = false; change.stage = 0; Serial.print("DONE CHANGE "); Serial.println(change.amt);
    }
  }
  
  // Check for stale change operations - reset if stuck for too long
  if (change.active && (millis() - lastChangeActivity > CHANGE_TIMEOUT_MS)) {
    stop_all_change();
    Serial.println("ERR CHANGE_TIMEOUT");
  }

  // Read & handle one line of serial command (non-blocking)
  String line = readLine();
  if (!line.length()) return;

  line.trim();
  int sp = line.indexOf(' ');
  String cmd  = (sp < 0) ? line : line.substring(0, sp);
  String rest = (sp < 0) ? ""   : line.substring(sp + 1);

  // -------- Commands --------
  if (cmd == "COINS?") {
    // Reply with inserted coin total
    Serial.println(acceptor.total());
  }
  else if (cmd == "COINS=RST") {
    acceptor.reset();
  }
  else if (cmd == "HOPPER") {
    if (hop1.busy() || hop5.busy() || hop10.busy() || change.active) { Serial.println("ERR BUSY"); return; }
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) { Serial.println("ERR BADARG"); return; }
    int denom = rest.substring(0, sp2).toInt();
    int n     = rest.substring(sp2 + 1).toInt();
    bool ok = false;
    if      (denom == 10) ok = hop10.start(n);
    else if (denom == 5 ) ok = hop5.start(n);
    else if (denom == 1 ) ok = hop1.start(n);
    else { Serial.println("ERR BADARG"); return; }
    if (!ok) Serial.println("ERR BUSY");
  }
  else if (cmd == "CHANGE") {
    // First ensure we're in a clean state
    if (hop1.busy() || hop5.busy() || hop10.busy() || change.active) {
      // Try to reset the system if it's stuck
      stop_all_change();
      
      // Check again after reset attempt
      if (hop1.busy() || hop5.busy() || hop10.busy() || change.active) {
        Serial.println("ERR BUSY"); 
        return;
      }
    }
    
    int amt = rest.toInt();
    if (amt < 0) { Serial.println("ERR BADARG"); return; }
    
    // Fresh start of change dispensing
    start_change((uint16_t)amt);
  }
  else if (cmd == "STOP") {
    stop_all_change();
    Serial.println("OK STOPPED");
  }
  else if (cmd == "RESET") {
    // Full system reset command
    stop_all_change();
    acceptor.reset();
    Serial.println("OK RESET");
  }
  else if (cmd == "PAPER") {
    // PAPER SHORT <n> | PAPER LONG <n>
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) { Serial.println("ERR BADARG"); return; }
    String which = rest.substring(0, sp2);
    uint16_t n   = (uint16_t)rest.substring(sp2 + 1).toInt();
    if (n == 0) { Serial.println("ERR BADARG"); return; }

    if      (which == "SHORT") dispShort.dispenseSheets(n);
    else if (which == "LONG")  dispLong.dispenseSheets(n);
    else { Serial.println("ERR BADARG"); return; }
  }
  else if (cmd == "STATUS?") {
    // Simple snapshot; expand if you want more
    Serial.print("C=");
    Serial.print(acceptor.total());
    Serial.print(" BUSY=");
    Serial.println((hop1.busy()||hop5.busy()||hop10.busy()||change.active) ? 1 : 0);
  }
  else {
    Serial.println("ERR BADARG");
  }
}
