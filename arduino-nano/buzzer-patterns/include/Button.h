#pragma once

#include <stdint.h>

// Debounced push button (pure). Feed the raw "pressed" state once per frame; returns true once
// on each fresh press (rising edge) after DEBOUNCE consistent samples. Hardware-free (no
// Arduino/AVR headers) so it unit-tests off-device. Generic -- not buzzer-specific, reusable by
// any project needing debounced button input. Same pattern as
// arduino-nano/led-dimmer-ws2812's Button class and arduino-nano/stepper's Button.h.
namespace input {

class Button {
 public:
  bool pressed(bool raw) {
    if (raw == state_) {
      count_ = 0;
      return false;
    }
    if (++count_ < DEBOUNCE) return false;  // not yet stable
    count_ = 0;
    state_ = raw;   // commit the new debounced state
    return state_;  // a fresh press only (true when committing pressed)
  }

 private:
  static constexpr uint8_t DEBOUNCE = 3;
  bool state_ = false;  // committed state: false = released
  uint8_t count_ = 0;   // consecutive samples differing from state_
};

}  // namespace input
