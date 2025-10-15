// Config.h
#pragma once

// ====== COIN SLOT INPUT ======
#define PIN_COIN_PULSE  2         /* e.g. 2 (INT0) */

// Pulses-to-value mapping (adjust if your acceptor differs)
#define PULSES_P1   1   // 1 peso
#define PULSES_P5   3   // 5 pesos
#define PULSES_P10  6   // 10 pesos
#define PULSES_P20  9   // 20 pesos
#define COIN_BURST_TIMEOUT_MS  50  // idle gap to consider burst ended

// ====== COIN HOPPERS (ACTIVE-LOW) ======
#define PIN_HOPPER_P1          4
#define PIN_HOPPER_P5          5
#define PIN_HOPPER_P10         6
// Hopper coin-out sensors (ezButton inputs)
#define PIN_HOPPER_P1_SENS     8
#define PIN_HOPPER_P5_SENS     9
#define PIN_HOPPER_P10_SENS    10
#define HOPPER_PULSE_MS        1000  // on-time per coin (tune to your mech)
#define HOPPER_COOL_MS         500   // cooling time between pulses
#define HOPPER_MAX_GAP_MS      3000

// ====== PAPER: SHORT STEPPER + DC ======
#define SHORT_PIN_DIR             22
#define SHORT_PIN_PULSE           23
#define SHORT_LIMIT_SWITCH        24
#define SHORT_DC_IN1              25
#define SHORT_DC_IN2              26
#define SHORT_DC_EN               44
#define SHORT_STEPS_PER_SHEET     1900
#define SHORT_STEP_US             900    // microseconds high/low

// ====== PAPER: LONG STEPPER + DC ======
#define LONG_PIN_DIR             28
#define LONG_PIN_PULSE           29
#define LONG_LIMIT_SWITCH        30
#define LONG_DC_IN1              31
#define LONG_DC_IN2              32
#define LONG_DC_EN               45
#define LONG_STEPS_PER_SHEET     2200
#define LONG_STEP_US             500 // 1000 microseconds delay (1ms)

// DC reverse clear timing
#define RAMP_DOWN_MS           8000

// General
#define DIR_CW   true
#define DIR_CCW  false
