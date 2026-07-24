#include <Arduino.h>
#include <AccelStepper.h>

#include "Stepper.h"

namespace {

// Pin assignments. ENABLE/MS1/MS2/MS3 are hardwired to GND on the driver board (always enabled,
// full-step) rather than driven by the MCU -- this project is about step-pulse timing, not
// microstepping, so keeping those off the pin list keeps the firmware to just pot + DIR + STEP.
constexpr uint8_t PIN_POT = A0;
constexpr uint8_t PIN_DIR = 2;
constexpr uint8_t PIN_STEP = 3;

constexpr uint8_t EMA_SHIFT = 3;  // pot smoothing strength

// Stepper strategy: Library (AccelStepper's constant-speed runSpeed(), which handles step timing
// for you) vs DirectPulse (from-scratch micros()-timed step generation, no library, no
// acceleration curve -- see README's "Stepper strategy" section for why both exist).
enum class StepMode : uint8_t { Library, DirectPulse };
constexpr StepMode STEP_MODE = StepMode::Library;

// Serial/Teleplot trace of raw -> speed, off by default -- flip to true to debug.
constexpr bool     DEBUG_TRACE_ENABLED     = false;
constexpr uint32_t DEBUG_TRACE_INTERVAL_MS = 200;

uint16_t smoothed = 0;

AccelStepper accelStepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);
uint32_t lastStepMicros = 0;
bool     stepPinHigh    = false;

// AccelStepper's constant-speed mode: no acceleration curve, just steps at a fixed rate until
// the speed changes. setSpeed() takes signed steps/sec (negative = reverse); this project only
// spins one direction, so speed is always >= 0.
void renderLibrary(uint16_t speed) {
  accelStepper.setSpeed(speed);
  accelStepper.runSpeed();
}

// From-scratch step-pulse generator: toggles STEP high/low at stepIntervalMicros(speed), timed
// entirely by micros() -- no AccelStepper involved. speed == 0 holds STEP low (motor at rest).
void renderDirectPulse(uint16_t speed) {
  if (speed == 0) {
    digitalWrite(PIN_STEP, LOW);
    return;
  }
  if (micros() - lastStepMicros >= stepper::stepIntervalMicros(speed)) {
    lastStepMicros = micros();
    stepPinHigh = !stepPinHigh;
    digitalWrite(PIN_STEP, stepPinHigh ? HIGH : LOW);
  }
}

// Teleplot-format trace (raw pot -> speed), throttled so it doesn't flood Serial.
void logDebugTrace(uint16_t raw, uint16_t speed) {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < DEBUG_TRACE_INTERVAL_MS) return;
  lastPrintMs = millis();
  Serial.print(">raw:");   Serial.println(raw);
  Serial.print(">speed:"); Serial.println(speed);
}

}  // namespace

void setup() {
  if (DEBUG_TRACE_ENABLED) Serial.begin(115200);

  pinMode(PIN_POT, INPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
  digitalWrite(PIN_DIR, HIGH);  // fixed rotation direction

  smoothed = analogRead(PIN_POT);  // start at the pot's actual position, not ramped up from 0

  accelStepper.setMaxSpeed(stepper::MAX_SPEED_STEPS_PER_SEC + 1);  // headroom above setSpeed()'s max
}

void loop() {
  uint16_t raw = analogRead(PIN_POT);
  smoothed = stepper::emaStep(smoothed, raw, EMA_SHIFT);
  uint16_t speed = stepper::potToSpeed(smoothed);

  if (STEP_MODE == StepMode::Library) renderLibrary(speed);
  else renderDirectPulse(speed);

  if (DEBUG_TRACE_ENABLED) logDebugTrace(raw, speed);
}
