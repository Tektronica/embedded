#pragma once

#include <stdint.h>

// A time-of-day clock: seconds since midnight, advanced by ticks. Hardware-free so it unit-tests
// off-device. Delegated to by Microwave.h's Controller rather than absorbed into it, since
// time-keeping is a distinct concern from the cook-timer state machine that uses it.
//
// There is no RTC chip in the BOM, so this is a plain software counter that resets to 0:00 on
// every power loss or reset. Fine for a toy/prop build; a battery-backed RTC (e.g. DS3231) would
// be needed for the clock to survive a power cycle.
//
// Namespace is `wallclock`, not `clock`, to avoid shadowing the C standard library's clock().
namespace wallclock {

constexpr uint32_t SECONDS_PER_DAY = 24UL * 60 * 60;

class Clock {
 public:
  uint16_t minutesOfDay() const { return static_cast<uint16_t>(secondsOfDay_ / 60); }

  void tick() { secondsOfDay_ = (secondsOfDay_ + 1) % SECONDS_PER_DAY; }

  void setMinutesOfDay(uint16_t minutes) {
    secondsOfDay_ = static_cast<uint32_t>(minutes) * 60;
  }

 private:
  uint32_t secondsOfDay_ = 0;
};

// Shift a new digit into a raw HH:MM entry buffer the way a real keypad does (each press
// appends to the low end, shifting existing digits left, e.g. 0 -> 8 -> 81 -> 815 builds the
// digit buffer "815"). Holds the raw digits, not a decoded time -- decode with
// decodeEnteredMinutes() below. Kept raw here (rather than decoding on every keystroke) because
// decoding clamps, and feeding a clamped value back into the next shift would corrupt later
// digits once an earlier field clamps (e.g. typing "9","9","3","0" must still read as 23:30 wide
// open to the last two digits, not lose them to an early clamp).
inline uint16_t nextEnteredMinutes(uint16_t enteredDigits, uint8_t digit) {
  return static_cast<uint16_t>((enteredDigits * 10 + digit) % 10000);
}

// Decode a raw digit buffer (as built by nextEnteredMinutes) into hours*60+minutes, clamping
// hours to 23 and minutes to 59 rather than carrying overflow, since a real keypad doesn't
// auto-carry either.
inline uint16_t decodeEnteredMinutes(uint16_t enteredDigits) {
  uint8_t hours = static_cast<uint8_t>(enteredDigits / 100);
  uint8_t minutes = static_cast<uint8_t>(enteredDigits % 100);
  if (hours > 23) hours = 23;
  if (minutes > 59) minutes = 59;
  return static_cast<uint16_t>(hours) * 60 + minutes;
}

}  // namespace wallclock
