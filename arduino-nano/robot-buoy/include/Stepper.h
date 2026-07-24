#pragma once

#include <stdint.h>
#include <Arduino.h>
#include <AccelStepper.h>

// Thin driver around a DRV8825-driven bipolar stepper: owns the AccelStepper instance and the
// driver's enable/microstep pins, exposing simple "drive toward a target position, report
// whether still moving" semantics. Generic -- knows nothing about what the motor is turning for
// (see Ballast.h for that). Hardware-coupled (AccelStepper calls micros() internally), so this
// doesn't unit-test off-device.
namespace stepper {

class Driver {
 public:
  Driver(uint8_t stepPin, uint8_t directionPin)
      : motor_(AccelStepper::DRIVER, stepPin, directionPin) {}

  void begin(uint8_t enablePin, uint8_t ms1Pin, uint8_t ms2Pin, uint8_t ms3Pin,
             float maxSpeedStepsPerSec, float accelerationStepsPerSec2) {
    motor_.setMaxSpeed(maxSpeedStepsPerSec);
    motor_.setAcceleration(accelerationStepsPerSec2);

    pinMode(enablePin, OUTPUT);
    pinMode(ms1Pin, OUTPUT);
    pinMode(ms2Pin, OUTPUT);
    pinMode(ms3Pin, OUTPUT);
    digitalWrite(ms1Pin, HIGH);
    digitalWrite(ms2Pin, HIGH);
    digitalWrite(ms3Pin, HIGH);    // 1/32 microstepping
    digitalWrite(enablePin, LOW);  // DRV8825 enable is active-low
  }

  // Drives toward targetSteps; returns true while still moving.
  bool driveTo(long targetSteps) {
    if (!motor_.isRunning()) motor_.moveTo(targetSteps);
    motor_.run();
    return motor_.isRunning();
  }

 private:
  AccelStepper motor_;
};

}  // namespace stepper
