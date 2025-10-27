#pragma once

#define COINS_PULSE_PIN 2
#define BUZZER_PIN 12
#define RELAY1_PIN 4
#define RELAY2_PIN 5
#define RELAY3_PIN 6

#define ONE_SENSOR_PIN 8
#define FIVE_SENSOR_PIN 9
#define TEN_SENSOR_PIN 10

// Motor configuration structures
struct StepperConfig {
  int dir_pin;
  int pulse_pin;
  int limit_switch;
  int steps;
};

struct DCMotorConfig {
  int IN1;
  int IN2;
  int en_pin;
};

struct MotorSystem {
  StepperConfig stepper;
  DCMotorConfig dc_motor;
};

// Setup Variables - static ensures internal linkage
static const MotorSystem a4_step_motors = {
  .stepper = {
    .dir_pin = 22,        // Digital pin 22
    .pulse_pin = 23,      // Digital pin 23
    .limit_switch = 24,   // Digital pin 24
    .steps = 1900
  },
  .dc_motor = {
    .IN1 = 25,           // Digital pin 25
    .IN2 = 26,           // Digital pin 26
    .en_pin = 44         // Digital pin 44 (PWM capable)
  }
};
  
static const MotorSystem long_step_motors = {
  .stepper = {
    .dir_pin = 28,        // Digital pin 28
    .pulse_pin = 29,      // Digital pin 29
    .limit_switch = 30,   // Digital pin 30
    .steps = 2200
  },
  .dc_motor = {
    .IN1 = 31,           // Digital pin 31
    .IN2 = 32,           // Digital pin 32
    .en_pin = 45         // Digital pin 45
  }
};