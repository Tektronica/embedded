#pragma once

#include <stdint.h>

// Hardware-free conditioning for a raw 10-bit potentiometer ADC reading: EMA smoothing, a
// deadzone that pins both ends of travel to the rails, and scaling to a percentage.
namespace potinput {

constexpr uint16_t ADC_MAX     = 1023;
constexpr uint8_t  PERCENT_MAX = 100;

constexpr uint16_t DEADZONE_DEFAULT_LOW        = 20;    // raw <= this -> 0
constexpr uint16_t DEADZONE_DEFAULT_HIGH       = 1000;  // raw >= this -> ADC_MAX
constexpr uint16_t DEADZONE_DEFAULT_HYSTERESIS = 10;    // counts past a threshold to leave its rail

// One EMA step toward `raw`; larger `shift` = smoother/slower. The `step != 0` guard removes the
// integer dead-band so it converges exactly to the rails.
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  int16_t step = static_cast<int16_t>(delta >> shift);
  if (step == 0 && delta != 0) step = (delta > 0) ? 1 : -1;
  return static_cast<uint16_t>(smoothed + step);
}

// Raw reading (0..ADC_MAX, clamped) -> 0..PERCENT_MAX.
inline uint8_t toPercent(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint8_t>(static_cast<uint32_t>(raw) * PERCENT_MAX / ADC_MAX);
}

// Pins readings near either end of travel to 0 / ADC_MAX. A rail is entered at its threshold but
// only left `hysteresis` counts past it, so noise on a threshold can't toggle the output. One
// instance per pot.
class Deadzone {
 public:
  explicit Deadzone(uint16_t low = DEADZONE_DEFAULT_LOW, uint16_t high = DEADZONE_DEFAULT_HIGH,
                    uint16_t hysteresis = DEADZONE_DEFAULT_HYSTERESIS)
      : low_(low), high_(high), hysteresis_(hysteresis) {}

  uint16_t apply(uint16_t raw) {
    if (atLow_) {
      atLow_ = raw < low_ + hysteresis_;
    } else {
      atLow_ = raw <= low_;
    }

    if (atHigh_) {
      atHigh_ = raw > high_ - hysteresis_;
    } else {
      atHigh_ = raw >= high_;
    }

    if (atLow_) return 0;
    if (atHigh_) return ADC_MAX;
    return raw;
  }

 private:
  uint16_t low_;
  uint16_t high_;
  uint16_t hysteresis_;
  bool atLow_ = false;
  bool atHigh_ = false;
};

}  // namespace potinput
