#include <Arduino.h>
#include <AccelStepper.h>

#include "Button.h"
#include "Stepper.h"

namespace {

// Pin assignments. ENABLE/MS1/MS2/MS3 are hardwired to GND on the driver board (always enabled,
// full-step) rather than driven by the MCU -- this project is about step-pulse timing, not
// microstepping, so keeping those off the pin list keeps the firmware to just pot + DIR + STEP +
// two buttons.
constexpr uint8_t PIN_POT = A0;
constexpr uint8_t PIN_DIR = 2;
constexpr uint8_t PIN_STEP = 3;
constexpr uint8_t PIN_BTN_RUN_STOP = 4;
constexpr uint8_t PIN_BTN_DIRECTION = 5;

constexpr uint8_t EMA_SHIFT = 3;  // pot smoothing strength

// Stepper strategy: Library (AccelStepper's moveTo()/run(), ramped by setAcceleration() -- see
// renderLibrary() below) vs DirectPulse (from-scratch micros()-timed step generation, no library,
// no acceleration curve -- see README's "Stepper strategy" section for why both exist).
enum class StepMode : uint8_t { Library, DirectPulse };
constexpr StepMode STEP_MODE = StepMode::Library;

constexpr float STEPPER_ACCELERATION = 500;  // steps/sec^2 -- ramp rate for Library mode
// Retargeted far ahead in the current direction so run() always has room to ramp up to cruise
// speed without ever actually arriving -- see renderLibrary()'s comment for why.
constexpr long FAR_STEPS = 2000000L;

// Serial/Teleplot trace of raw -> speed, off by default -- flip to true to debug.
constexpr bool     DEBUG_TRACE_ENABLED     = false;
constexpr uint32_t DEBUG_TRACE_INTERVAL_MS = 200;

uint16_t smoothed = 0;
bool     running   = false;  // starts stopped -- safer default than spinning on power-up
bool     clockwise = true;

input::Button runStopButton;
input::Button directionButton;

AccelStepper accelStepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);
bool         lastClockwise = true;  // direction the current moveTo() target was set for
uint32_t     lastStepMicros = 0;
bool         stepPinHigh    = false;

// Continuous, speed-controlled rotation with a proper accel/decel ramp -- including on direction
// reversal -- built entirely from AccelStepper's moveTo()/run(), not a custom ramp. The trick:
// run() only ramps speed *while approaching a moveTo() target*, so "spin indefinitely" means
// retargeting far ahead in the current direction and never actually arriving. On a direction
// flip (or once the target gets close), retarget far the other way; run() then decelerates to a
// stop at setAcceleration()'s rate and accelerates the other way -- the same mechanism that
// would otherwise just slow down to arrive at a real target. setMaxSpeed() is updated every call
// so the pot continuously retargets the cruise speed, which also means stopping (speed 0) ramps
// down smoothly instead of cutting the motor immediately.
//
// AccelStepper owns the DIR pin itself for the DRIVER interface -- this never writes to it
// directly (that would fight the library for control of the pin).
void renderLibrary(uint16_t speed, bool clockwise) {
  if (clockwise != lastClockwise || labs(accelStepper.distanceToGo()) < FAR_STEPS / 10) {
    lastClockwise = clockwise;
    accelStepper.moveTo(accelStepper.currentPosition() + (clockwise ? FAR_STEPS : -FAR_STEPS));
  }
  accelStepper.setMaxSpeed(speed);
  accelStepper.run();
}

// From-scratch step-pulse generator: toggles STEP high/low at stepIntervalMicros(speed), timed
// entirely by micros() -- no AccelStepper involved, so DIR is ours to drive directly. speed == 0
// holds STEP low (motor at rest).
void renderDirectPulse(uint16_t speed, bool clockwise) {
  digitalWrite(PIN_DIR, clockwise ? HIGH : LOW);
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

// Teleplot-format trace (raw pot -> speed, plus run/direction state), throttled so it doesn't
// flood Serial.
void logDebugTrace(uint16_t raw, uint16_t speed) {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < DEBUG_TRACE_INTERVAL_MS) return;
  lastPrintMs = millis();
  Serial.print(">raw:");       Serial.println(raw);
  Serial.print(">speed:");     Serial.println(speed);
  Serial.print(">running:");   Serial.println(running);
  Serial.print(">clockwise:"); Serial.println(clockwise);
}

}  // namespace

void setup() {
  if (DEBUG_TRACE_ENABLED) Serial.begin(115200);

  pinMode(PIN_POT, INPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_BTN_RUN_STOP, INPUT_PULLUP);
  pinMode(PIN_BTN_DIRECTION, INPUT_PULLUP);

  smoothed = analogRead(PIN_POT);  // start at the pot's actual position, not ramped up from 0

  accelStepper.setAcceleration(STEPPER_ACCELERATION);
  accelStepper.moveTo(FAR_STEPS);  // initial target; renderLibrary() retargets as needed
}

void loop() {
  if (runStopButton.pressed(digitalRead(PIN_BTN_RUN_STOP) == LOW)) running = !running;
  if (directionButton.pressed(digitalRead(PIN_BTN_DIRECTION) == LOW)) clockwise = !clockwise;

  uint16_t raw = analogRead(PIN_POT);
  smoothed = stepper::emaStep(smoothed, raw, EMA_SHIFT);
  uint16_t speed = running ? stepper::potToSpeed(smoothed) : 0;

  if (STEP_MODE == StepMode::Library) renderLibrary(speed, clockwise);
  else renderDirectPulse(speed, clockwise);

  if (DEBUG_TRACE_ENABLED) logDebugTrace(raw, speed);
}
