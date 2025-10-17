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
  
  if (n <= 0) {
    Serial.print("Hopper ");
    Serial.print(denom_);
    Serial.println(" Invalid dispense amount");
    return false;
  }
  
  // Reset and clear button state first
  if (btn_ != nullptr) {
    // Ensure button state is cleared before starting
    for (int i = 0; i < 5; i++) {
      btn_->loop();
      delay(5);
    }
  }
  
  target_   = n;
  count_    = 0;
  busy_     = true;
  lastCoin_ = millis();

  // initial push
  on_();
  pulsing_    = true;
  pulseStart_ = millis();
  
  Serial.print("Hopper ");
  Serial.print(denom_);
  Serial.print(" Started dispensing ");
  Serial.print(n);
  Serial.println(" coins");
  
  // Short delay to ensure the actuator has time to activate before we start checking for coins
  delay(100);
  
  return true;
}

void HopperPayout::stop() {
  off_();
  busy_    = false;
  pulsing_ = false;
  Serial.print("Hopper ");
  Serial.print(denom_);
  Serial.println(" Stopped dispensing");
}

void HopperPayout::reset() {
  count_ = 0;
  target_ = 0;
  busy_ = false;
  pulsing_ = false;
  lastCoin_ = 0;
  off_();
  
  // Reset and clear the button state
  if (btn_ != nullptr) {
    // Force the button to update its state
    for (int i = 0; i < 5; i++) {
      btn_->loop();
      delay(5);  // Brief delay to allow the button to settle
    }
  }
  
  Serial.print("Hopper ");
  Serial.print(denom_);
  Serial.println(" Reset");
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

  // Coin detection with improved algorithm
  bool currentState = btn_->isPressed();
  
  // Only count when the button state changes from not pressed to pressed
  // This prevents false triggers at the start of dispensing
  if (currentState && !lastButtonState_ && (now - pulseStart_ > 150)) {
    count_++;
    lastCoin_ = now;
    
    // Print the count information
    Serial.print("OUT ");
    Serial.print(denom_);
    Serial.print(" Count: ");
    Serial.print(count_);
    
    if (busy_ && target_ > 0) {
      Serial.print("/");
      Serial.print(target_);
    }
    Serial.println();
    
    // Check if target is reached
    if (busy_ && target_ > 0 && count_ >= target_) {
      stop();
      // Print standard completion message first
      Serial.println("DONE HOPPER");
      // Then print detailed information
      Serial.print("DONE ");
      Serial.print(denom_);
      Serial.print(" TARGET REACHED! ");
      Serial.println(count_);
    }
  }
  
  // Update the last button state
  lastButtonState_ = currentState;
  
  // Timeout watchdog
  if (busy_) {
    if ((now - lastCoin_) >= HOPPER_MAX_GAP_MS) {
      stop();
      // Print standard completion message first
      Serial.println("DONE HOPPER");
      // Then print error information
      Serial.print("ERR TIMEOUT ");
      Serial.print(denom_);
      Serial.print(" Final count: ");
      Serial.print(count_);
      Serial.print("/");
      Serial.println(target_);
    }
  }
}

bool HopperPayout::busy() const { return busy_; }

void HopperPayout::on_()  { digitalWrite(pins_.actuatorPin, pins_.activeLow ? LOW  : HIGH); }
void HopperPayout::off_() { digitalWrite(pins_.actuatorPin, pins_.activeLow ? HIGH : LOW ); }
