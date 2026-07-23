#pragma once

#include <stdint.h>

// Potentiometer → PWM duty-cycle controller. Hardware-free (no Arduino/AVR headers) so it
// unit-tests off-device. One pot sets duty (0..100%), rendered by one of two firmware variants:
// main.cpp (programmatic Timer1 frequency control) or main_wokwi.cpp (analogWrite(), fixed
// frequency, for Wokwi simulation — see README's "Two firmware variants" section for why both exist).
namespace pwm {

constexpr uint16_t ADC_MAX  = 1023;
constexpr uint8_t  DUTY_MAX = 100;
constexpr uint8_t  PWM_MAX  = 255;  // analogWrite()'s 8-bit range

// --- Potentiometer input ---

// Map a raw 10-bit ADC reading to a duty percentage (0..100).
inline uint8_t dutyFromAdc(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint8_t>(static_cast<uint32_t>(raw) * DUTY_MAX / ADC_MAX);
}

// One EMA step toward `raw`; larger `shift` = smoother/slower. The `step != 0` guard removes the
// integer dead-band so it converges exactly (pot at max really reaches 100% duty).
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  int16_t step = static_cast<int16_t>(delta >> shift);
  if (step == 0 && delta != 0) step = (delta > 0) ? 1 : -1;
  return static_cast<uint16_t>(smoothed + step);
}

// --- Perceived-brightness correction ---

// Square-law gamma correction (~2.0): LEDs (and the eye) respond to duty cycle on a curve, so a
// linear duty sweep looks maxed out well before the pot reaches its end. Squaring the percentage
// keeps low pot positions dim and saves the steepest brightness ramp for the top of the sweep.
inline uint8_t gammaCorrect(uint8_t percent) {
  if (percent > DUTY_MAX) percent = DUTY_MAX;
  return static_cast<uint8_t>(static_cast<uint16_t>(percent) * percent / DUTY_MAX);
}

// --- Duty → analogWrite() value (main_wokwi.cpp) ---

// Duty percentage (0..100) → analogWrite() value (0..255).
inline uint8_t dutyToPwm8(uint8_t dutyPercent) {
  if (dutyPercent > DUTY_MAX) dutyPercent = DUTY_MAX;
  return static_cast<uint8_t>(static_cast<uint16_t>(dutyPercent) * PWM_MAX / DUTY_MAX);
}

// --- Frequency → Timer1 register math (main.cpp) ---

// Timer1 clock prescaler options available on the ATmega328 (CS12:CS10).
enum class Prescaler : uint8_t { Div1, Div8, Div64, Div256, Div1024 };

inline uint32_t prescalerValue(Prescaler p) {
  switch (p) {
    case Prescaler::Div1:    return 1;
    case Prescaler::Div8:    return 8;
    case Prescaler::Div64:   return 64;
    case Prescaler::Div256:  return 256;
    case Prescaler::Div1024: return 1024;
  }
  return 1;
}

// Fast-PWM register config for a target frequency: freq = cpuHz / (prescaler * (top+1)).
struct TimerConfig {
  Prescaler prescaler;
  uint16_t  top;
};

// Pick the smallest prescaler whose TOP value fits the 16-bit timer — smaller prescalers give
// finer duty resolution. Falls back to the largest prescaler (clamped) for very low frequencies.
inline TimerConfig computeTimerConfig(uint32_t cpuHz, uint32_t targetHz) {
  constexpr Prescaler kOptions[] = {Prescaler::Div1, Prescaler::Div8, Prescaler::Div64,
                                     Prescaler::Div256, Prescaler::Div1024};
  for (Prescaler p : kOptions) {
    uint32_t top = cpuHz / (prescalerValue(p) * targetHz);
    if (top >= 1 && top <= 65536) return TimerConfig{p, static_cast<uint16_t>(top - 1)};
  }
  return TimerConfig{Prescaler::Div1024, 65535};
}

// Duty percentage (0..100) → OCR1A compare value for a given TOP. 100% uses top+1 (beyond TOP) so
// the compare match never fires and the pin stays fully high, avoiding the one-tick-low glitch
// that OCR1A == TOP would otherwise cause each period (mirrors the OCR1A == 0 glitch at 0%).
inline uint16_t dutyToOcr(uint8_t dutyPercent, uint16_t top) {
  if (dutyPercent > DUTY_MAX) dutyPercent = DUTY_MAX;
  uint32_t ocr = (static_cast<uint32_t>(top) + 1) * dutyPercent / DUTY_MAX;
  // top == 0xFFFF: top+1 needs 17 bits, which can't be written to the 16-bit OCR1A register at
  // all. Settle for top itself (the one-tick glitch) instead of letting the cast below wrap to 0.
  if (ocr > 0xFFFFu) ocr = top;
  return static_cast<uint16_t>(ocr);
}

}  // namespace pwm
