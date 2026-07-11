#include <Arduino.h>

#include "Pwm.h"

namespace {

// Pin assignments.
constexpr uint8_t PIN_POT = A0;
constexpr uint8_t PIN_PWM = 9;  // D9 = OC1A (Timer1) for the direct-timer path; also a valid
                                 // analogWrite() pin for the fallback path.

// Output frequency for the direct-timer path — the one constant to change for a different frequency.
constexpr uint32_t PWM_FREQUENCY_HZ = 1000;
constexpr uint8_t  EMA_SHIFT        = 3;  // pot smoothing strength

// PWM strategy: AnalogWrite (fixed ~490 Hz; this project's LEDs look fine at that, and it's the
// only one Wokwi's avr8js core emulates correctly) vs DirectTimer (Timer1 register control for an
// arbitrary settable frequency, correct per the ATmega328 datasheet but not simulator-friendly —
// kept here as a reference implementation; see README's "PWM strategy" section).
enum class PwmMode : uint8_t { DirectTimer, AnalogWrite };
constexpr PwmMode PWM_MODE = PwmMode::AnalogWrite;

// Serial/Teleplot trace of raw -> duty, off by default — flip to true to debug.
constexpr bool     DEBUG_TRACE_ENABLED     = false;
constexpr uint32_t DEBUG_TRACE_INTERVAL_MS = 200;

uint16_t top      = 0;
uint16_t smoothed = 0;

// Maps the pure Prescaler choice to Timer1's CS12:CS10 bits (the one hardware-specific spot).
void applyPrescalerBits(pwm::Prescaler p) {
  switch (p) {
    case pwm::Prescaler::Div1:    TCCR1B |= _BV(CS10); break;
    case pwm::Prescaler::Div8:    TCCR1B |= _BV(CS11); break;
    case pwm::Prescaler::Div64:   TCCR1B |= _BV(CS11) | _BV(CS10); break;
    case pwm::Prescaler::Div256:  TCCR1B |= _BV(CS12); break;
    case pwm::Prescaler::Div1024: TCCR1B |= _BV(CS12) | _BV(CS10); break;
  }
}

// Timer1, Fast PWM mode 14 (WGM13:0 = 14): ICR1 holds TOP (frequency), OCR1A is the duty compare
// register, non-inverting output on OC1A.
void setupDirectTimerPwm() {
  pwm::TimerConfig cfg = pwm::computeTimerConfig(F_CPU, PWM_FREQUENCY_HZ);
  top = cfg.top;

  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12);
  applyPrescalerBits(cfg.prescaler);
  ICR1  = top;
  OCR1A = 0;
}

void renderDirectTimerPwm(uint8_t duty) { OCR1A = pwm::dutyToOcr(duty, top); }

// analogWrite() fallback: fixed ~490 Hz on D9/D10, but the only path Wokwi simulates correctly.
void renderAnalogWritePwm(uint8_t duty) { analogWrite(PIN_PWM, pwm::dutyToPwm8(duty)); }

// Teleplot-format trace (raw pot -> duty), throttled so it doesn't flood Serial.
void logDebugTrace(uint16_t raw, uint8_t duty) {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < DEBUG_TRACE_INTERVAL_MS) return;
  lastPrintMs = millis();
  Serial.print(">raw:");  Serial.println(raw);
  Serial.print(">duty:"); Serial.println(duty);
}

}  // namespace

void setup() {
  if (DEBUG_TRACE_ENABLED) Serial.begin(115200);

  pinMode(PIN_POT, INPUT);
  pinMode(PIN_PWM, OUTPUT);

  smoothed = analogRead(PIN_POT);  // start at the pot's actual position, not ramped up from 0

  if (PWM_MODE == PwmMode::DirectTimer) setupDirectTimerPwm();
}

void loop() {
  // read pot input -> duty (linear; see Pwm.h's gammaCorrect() for real-LED/eye perceptual correction)
  uint16_t raw = analogRead(PIN_POT);
  smoothed = pwm::emaStep(smoothed, raw, EMA_SHIFT);
  uint8_t duty = pwm::dutyFromAdc(smoothed);

  // render duty -> PWM output
  if (PWM_MODE == PwmMode::DirectTimer) renderDirectTimerPwm(duty);
  else renderAnalogWritePwm(duty);

  if (DEBUG_TRACE_ENABLED) logDebugTrace(raw, duty);
}
