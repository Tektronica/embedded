#pragma once

#include <stdint.h>

#include "Clock.h"

// Top-level control-flow state machine: Idle -> Setting -> Running -> Done, plus ClockSet
// (reachable from Idle) for setting the time-of-day clock Idle displays. Setting/Running/Done
// serve two distinct Functions -- CookTime and Timer (a plain kitchen timer) -- sharing one flow
// rather than duplicating it, since digit entry, Start/Cancel, and counting down to Done are
// identical either way; only isCooking() (CookTime + Running) distinguishes them for main.cpp's
// hardware gating. Coordinates the display, buzzer, motor, fan, and light without touching any
// hardware directly, so it unit-tests off-device. Takes abstract input events rather than reading
// any specific keypad wiring, so it doesn't need to wait on the matrix-vs-individual-switches
// decision — main.cpp maps real key reads to Event and real peripheral writes to the reported
// State.
//
// This is the app-level orchestrator: it owns the cook-timer/kitchen-timer flow itself but
// delegates time-of-day keeping and countdown-counting to wallclock::Clock/wallclock::Timer
// (Clock.h) rather than absorbing either concern directly.
namespace microwave {

enum class State : uint8_t { Idle, Setting, Running, Done, ClockSet };
enum class EventType : uint8_t { Digit, Start, Cancel, Clock, Timer, Tick };

// What an active Setting/Running/Done countdown is for -- orthogonal to State, since the digit
// entry, Start/Cancel handling, and countdown-to-Done logic are identical either way. Only the
// hardware side (main.cpp's isCooking()-gated motor/fan/light/hum) cares which one it is.
enum class Function : uint8_t { CookTime, Timer };

struct Event {
  EventType type;
  uint8_t   digit;  // valid only when type == EventType::Digit, 0..9
};

// Shift a new digit into a raw MM:SS entry buffer the way a real keypad does (each press
// appends to the low end, shifting existing digits left, e.g. 0 -> 3 -> 30 -> 305 -> 3053 builds
// the digit buffer "3053"). Holds the raw digits, not a decoded time -- decode with
// decodeEnteredSeconds() below, so an earlier field clamping can't corrupt digits typed after it.
inline uint16_t nextEnteredSeconds(uint16_t enteredDigits, uint8_t digit) {
  return static_cast<uint16_t>((enteredDigits * 10 + digit) % 10000);
}

// Decode a raw digit buffer (as built by nextEnteredSeconds) into total seconds. Minutes aren't
// clamped beyond the natural 4-digit bound (99); seconds clamp to 59 rather than carrying
// overflow into minutes, since a real keypad doesn't auto-carry either.
inline uint16_t decodeEnteredSeconds(uint16_t enteredDigits) {
  uint8_t minutes = static_cast<uint8_t>(enteredDigits / 100);
  uint8_t seconds = static_cast<uint8_t>(enteredDigits % 100);
  if (seconds > 59) seconds = 59;
  return static_cast<uint16_t>(minutes) * 60 + seconds;
}

class Controller {
 public:
  State state() const { return state_; }

  // True only while an actual cook is running -- main.cpp gates the motor/fan/light/hum on this
  // rather than bare Running, since Running also covers a plain kitchen-timer countdown, which
  // shouldn't turn any of that hardware on.
  bool isCooking() const { return state_ == State::Running && function_ == Function::CookTime; }

  // The value to show, where value/60 and value%60 are the two displayed digit pairs: the
  // current time-of-day (HH:MM, from the delegated Clock) while Idle, the in-progress entry
  // while Setting/ClockSet, or the countdown (cook time or kitchen timer, from the delegated
  // Timer) while Running/Done.
  uint16_t displayValue() const {
    switch (state_) {
      case State::Idle:     return clock_.minutesOfDay();
      case State::Setting:  return decodeEnteredSeconds(enteredDigits_);
      case State::ClockSet: return wallclock::decodeEnteredMinutes(enteredClockDigits_);
      case State::Running:
      case State::Done:
      default:              return timer_.remaining();
    }
  }

  void handle(Event event) {
    // The clock keeps running in the background regardless of state (cooking doesn't stop the
    // time of day) — except while it's being actively edited.
    if (event.type == EventType::Tick && state_ != State::ClockSet) {
      clock_.tick();
    }

    switch (state_) {
      case State::Idle:
        if (event.type == EventType::Digit) {
          function_ = Function::CookTime;
          enteredDigits_ = nextEnteredSeconds(0, event.digit);
          state_ = State::Setting;
        } else if (event.type == EventType::Timer) {
          function_ = Function::Timer;
          enteredDigits_ = 0;
          state_ = State::Setting;
        } else if (event.type == EventType::Clock) {
          enteredClockDigits_ = 0;
          state_ = State::ClockSet;
        }
        break;

      case State::Setting:
        if (event.type == EventType::Digit) {
          enteredDigits_ = nextEnteredSeconds(enteredDigits_, event.digit);
        } else if (event.type == EventType::Start) {
          uint16_t seconds = decodeEnteredSeconds(enteredDigits_);
          if (seconds > 0) {
            timer_.start(seconds);
            state_ = State::Running;
          }
        } else if (event.type == EventType::Cancel) {
          resetCountdown();
        }
        break;

      case State::Running:
        if (event.type == EventType::Tick) {
          timer_.tick();
          if (!timer_.isRunning()) state_ = State::Done;
        } else if (event.type == EventType::Cancel) {
          resetCountdown();
        }
        break;

      case State::Done:
        if (event.type == EventType::Cancel || event.type == EventType::Start ||
            event.type == EventType::Digit) {
          resetCountdown();
        }
        break;

      case State::ClockSet:
        if (event.type == EventType::Digit) {
          enteredClockDigits_ = wallclock::nextEnteredMinutes(enteredClockDigits_, event.digit);
        } else if (event.type == EventType::Start) {
          clock_.setMinutesOfDay(wallclock::decodeEnteredMinutes(enteredClockDigits_));
          state_ = State::Idle;
        } else if (event.type == EventType::Cancel) {
          state_ = State::Idle;
        }
        break;
    }
  }

 private:
  void resetCountdown() {
    state_ = State::Idle;
    enteredDigits_ = 0;
    timer_.cancel();
  }

  State            state_ = State::Idle;
  Function         function_ = Function::CookTime;  // which kind of countdown is active
  uint16_t         enteredDigits_ = 0;        // raw digit buffer while Setting; see decodeEnteredSeconds
  uint16_t         enteredClockDigits_ = 0;   // raw digit buffer while ClockSet; see wallclock::decodeEnteredMinutes
  wallclock::Clock clock_;                    // delegated time-of-day keeping
  wallclock::Timer timer_;                    // delegated countdown, cook time or kitchen timer
};

}  // namespace microwave
