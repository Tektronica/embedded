#pragma once

#include <stdint.h>

// Hardware-free duty-cycle math: a 0..100 duty percentage -> analogWrite() values or ATmega328
// Timer1 register values, plus a perceptual brightness curve.
namespace pwmduty {

constexpr uint8_t DUTY_MAX = 100;
constexpr uint8_t PWM8_MAX = 255;  // analogWrite()'s 8-bit range

// Square-law gamma correction (~2.0): LEDs and the eye respond to duty on a curve, so a linear
// sweep looks maxed out well before 100%.
inline uint8_t gammaCorrect(uint8_t percent) {
  if (percent > DUTY_MAX) percent = DUTY_MAX;
  return static_cast<uint8_t>(static_cast<uint16_t>(percent) * percent / DUTY_MAX);
}

// Duty percentage (0..100) -> analogWrite() value (0..255).
inline uint8_t dutyToPwm8(uint8_t dutyPercent) {
  if (dutyPercent > DUTY_MAX) dutyPercent = DUTY_MAX;
  return static_cast<uint8_t>(static_cast<uint16_t>(dutyPercent) * PWM8_MAX / DUTY_MAX);
}

// Timer1 clock prescaler options on the ATmega328 (CS12:CS10).
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

// Smallest prescaler whose TOP fits the 16-bit timer, for the finest duty resolution. Falls back
// to the largest prescaler (clamped) for very low frequencies.
inline TimerConfig computeTimerConfig(uint32_t cpuHz, uint32_t targetHz) {
  constexpr Prescaler kOptions[] = {Prescaler::Div1, Prescaler::Div8, Prescaler::Div64,
                                     Prescaler::Div256, Prescaler::Div1024};
  for (Prescaler p : kOptions) {
    uint32_t top = cpuHz / (prescalerValue(p) * targetHz);
    if (top >= 1 && top <= 65536) return TimerConfig{p, static_cast<uint16_t>(top - 1)};
  }
  return TimerConfig{Prescaler::Div1024, 65535};
}

// Duty percentage (0..100) -> OCR1A compare value for a given TOP. 100% uses top+1 so the compare
// never matches and the pin stays fully high, avoiding a one-tick-low glitch each period.
inline uint16_t dutyToOcr(uint8_t dutyPercent, uint16_t top) {
  if (dutyPercent > DUTY_MAX) dutyPercent = DUTY_MAX;
  uint32_t ocr = (static_cast<uint32_t>(top) + 1) * dutyPercent / DUTY_MAX;
  // top == 0xFFFF: top+1 doesn't fit the 16-bit register, so settle for top rather than wrap to 0.
  if (ocr > 0xFFFFu) ocr = top;
  return static_cast<uint16_t>(ocr);
}

}  // namespace pwmduty
