#include "HopperPayout.h"

void HopperPayout::begin(uint8_t denom, const HopperPins& pins, uint16_t debounceMs) {
  denom_ = denom;
  pins_  = pins;

  pinMode(pins_.actuatorPin, OUTPUT);
  off_(); // idle actuator

  btn_ = new ezButton(pins_.sensorPin);
  btn_->setDebounceTime(debounceMs);
}

bool HopperPayout::start(uint16_t n) {
  if (busy_) return false;
  target_   = n;
  count_    = 0;
  busy_     = (n > 0);
  lastCoin_ = millis();

  // initial push
  on_();
  pulsing_    = true;
  pulseStart_ = millis();
  return true;
}

void HopperPayout::stop() {
  off_();
  busy_    = false;
  pulsing_ = false;
}

void HopperPayout::loop() {
  if (!busy_) return;

  btn_->loop();
  const uint32_t now = millis();

  // Handle push ON window (HOPPER_PULSE_MS is taken from Config.h)
  if (pulsing_) {
    if (now - pulseStart_ >= HOPPER_PULSE_MS) {
      off_();
      pulsing_   = false;
      coolStart_ = now;
    }
  } else {
    // Cooling gap before next push
    if (now - coolStart_ >= HOPPER_COOL_MS) {
      // Kick again if we haven't just seen a coin
      if ((now - lastCoin_) >= 50) {
        on_();
        pulsing_    = true;
        pulseStart_ = now;
      }
    }
  }

  // Coin detection (flip to isReleased() if your sensor logic is inverted)
  if (btn_->isPressed()) {
    count_++;
    lastCoin_ = now;

    Serial.print("OUT ");
    Serial.println(denom_);

    if (count_ >= target_) {
      stop();
      Serial.print("DONE ");
      Serial.print(denom_);
      Serial.print(' ');
      Serial.println(count_);
    }
  }

  // Timeout watchdog
  if ((now - lastCoin_) >= HOPPER_MAX_GAP_MS) {
    stop();
    Serial.print("ERR TIMEOUT ");
    Serial.print(denom_);
    Serial.print(' ');
    Serial.print(count_);
    Serial.print('/');
    Serial.println(target_);
  }
}

bool HopperPayout::busy() const { return busy_; }

void HopperPayout::on_()  { digitalWrite(pins_.actuatorPin, pins_.activeLow ? LOW  : HIGH); }
void HopperPayout::off_() { digitalWrite(pins_.actuatorPin, pins_.activeLow ? HIGH : LOW ); }
