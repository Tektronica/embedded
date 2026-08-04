#pragma once

#include <stdint.h>

// Generic, hardware-free time-keeping building blocks -- the same trio a real RTC chip (e.g. the
// DS3231 named below) typically exposes: a time-of-day clock, a countdown timer, and (not yet
// needed by anything in this project, so not added speculatively) an alarm. Delegated to by
// Microwave.h's Controller rather than absorbed into it, since time-keeping is a distinct concern
// from the cook-timer/kitchen-timer state machine that uses it.
//
// There is no RTC chip in the BOM, so `Clock` is a plain software counter that resets to 0:00 on
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

// A generic countdown: start it with a duration, tick it down, and ask whether it's still
// running. Knows nothing about what should happen at zero -- that's the caller's business
// (a cook-timer state machine turning off a motor, a plain kitchen timer just beeping, or
// anything else that needs "count down N seconds and tell me when you hit zero").
class Timer {
 public:
  void start(uint16_t seconds) {
    remaining_ = seconds;
    running_ = seconds > 0;  // starting at 0 is immediately expired, not "running"
  }

  void cancel() {
    remaining_ = 0;
    running_ = false;
  }

  void tick() {
    if (!running_) return;
    if (--remaining_ == 0) running_ = false;
  }

  uint16_t remaining() const { return remaining_; }
  bool isRunning() const { return running_; }

 private:
  uint16_t remaining_ = 0;
  bool running_ = false;
};

// Shift a new digit into a raw 4-digit entry buffer the way a real keypad does (each press
// appends to the low end, shifting existing digits left, e.g. 0 -> 8 -> 81 -> 815 builds the
// digit buffer "815"). Generic across any HH:MM/MM:SS-style entry -- nextEnteredMinutes below and
// Microwave.h's nextEnteredSeconds are both thin, domain-named wrappers over this one primitive
// rather than duplicating its body. Holds the raw digits, not a decoded value -- decoding clamps,
// and feeding a clamped value back into the next shift would corrupt later digits once an earlier
// field clamps (e.g. typing "9","9","3","0" must still read as 23:30 wide open to the last two
// digits, not lose them to an early clamp).
inline uint16_t shiftEnteredDigit(uint16_t enteredDigits, uint8_t digit) {
  return static_cast<uint16_t>((enteredDigits * 10 + digit) % 10000);
}

inline uint16_t nextEnteredMinutes(uint16_t enteredDigits, uint8_t digit) {
  return shiftEnteredDigit(enteredDigits, digit);
}

// Split a raw 4-digit entry buffer (as built by shiftEnteredDigit) into its two 2-digit fields,
// unclamped -- callers decide policy on top of this: decodeEnteredMinutes below clamps for a live
// preview while typing, isValidTime rejects outright for a commit decision.
struct DigitFields {
  uint8_t high;
  uint8_t low;
};

inline DigitFields splitDigits(uint16_t enteredDigits) {
  return DigitFields{static_cast<uint8_t>(enteredDigits / 100),
                      static_cast<uint8_t>(enteredDigits % 100)};
}

// Decode a raw digit buffer (as built by nextEnteredMinutes) into hours*60+minutes, clamping
// hours to 23 and minutes to 59 rather than carrying overflow, since a real keypad doesn't
// auto-carry either. For a live preview while typing, not a validity check -- see isValidTime.
inline uint16_t decodeEnteredMinutes(uint16_t enteredDigits) {
  DigitFields fields = splitDigits(enteredDigits);
  uint8_t hours = fields.high > 23 ? 23 : fields.high;
  uint8_t minutes = fields.low > 59 ? 59 : fields.low;
  return static_cast<uint16_t>(hours) * 60 + minutes;
}

// Whether an hour:minute pair is being interpreted on a 24-hour or 12-hour clock face.
enum class TimeFormat : uint8_t { H24, H12 };

// True if hour:minute is a legal time in the given format. H24: hour 0-23 (midnight is 0).
// H12: hour 1-12 (a 12-hour face has no "0"). Doesn't track AM/PM either way -- a caller needing
// that distinction tracks the period separately, since validating an hour:minute pair doesn't
// require it.
inline bool isValidTime(uint8_t hour, uint8_t minute, TimeFormat format) {
  if (minute > 59) return false;

  switch (format) {
    case TimeFormat::H24:
      return hour <= 23;
    case TimeFormat::H12:
      return hour >= 1 && hour <= 12;
  }
  return false;  // unreachable, silences -Wreturn-type on some compilers
}

// Convert a 24-hour hour value (0-23) to its 12-hour-clock equivalent (1-12) -- midnight (0) and
// noon (12) both read as 12, same as any 12-hour clock face. Loses AM/PM, same as isValidTime.
inline uint8_t to12Hour(uint8_t hour24) {
  uint8_t h = hour24 % 12;
  return h == 0 ? 12 : h;
}

}  // namespace wallclock
