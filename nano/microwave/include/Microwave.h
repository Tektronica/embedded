#pragma once

#include <stdint.h>

// Top-level control-flow state machine: Idle -> Setting -> Running -> Done. Coordinates the
// display, buzzer, motor, fan, and light without touching any hardware directly, so it
// unit-tests off-device. Takes abstract input events rather than reading any specific keypad
// wiring, so it doesn't need to wait on the matrix-vs-individual-switches decision — main.cpp
// maps real key reads to Event and real peripheral writes to the reported State.
namespace microwave {

enum class State : uint8_t { Idle, Setting, Running, Done };
enum class EventType : uint8_t { Digit, Start, Cancel, Tick };

struct Event {
  EventType type;
  uint8_t   digit;  // valid only when type == EventType::Digit, 0..9
};

// Digit entry shifts left the way a real microwave keypad does: each digit press appends to the
// low end and the display reads it as MMSS, e.g. 0 -> 3 -> 30 -> 305 -> 3053 means 30 minutes
// 53 seconds. Kept to 4 digits; a seconds value above 59 gets clamped rather than carried into
// minutes, since a real microwave keypad doesn't auto-carry either.
inline uint16_t nextEnteredSeconds(uint16_t enteredSeconds, uint8_t digit) {
  uint16_t shifted = static_cast<uint16_t>((enteredSeconds * 10 + digit) % 10000);
  uint8_t minutes = static_cast<uint8_t>(shifted / 100);
  uint8_t seconds = static_cast<uint8_t>(shifted % 100);
  if (seconds > 59) seconds = 59;
  return static_cast<uint16_t>(minutes) * 60 + seconds;
}

class Controller {
 public:
  State state() const { return state_; }

  // Seconds to show: the in-progress entry while Setting, the countdown while Running/Done.
  uint16_t displaySeconds() const {
    return state_ == State::Setting ? enteredSeconds_ : remainingSeconds_;
  }

  void handle(Event event) {
    switch (state_) {
      case State::Idle:
        if (event.type == EventType::Digit) {
          enteredSeconds_ = nextEnteredSeconds(0, event.digit);
          state_ = State::Setting;
        }
        break;

      case State::Setting:
        if (event.type == EventType::Digit) {
          enteredSeconds_ = nextEnteredSeconds(enteredSeconds_, event.digit);
        } else if (event.type == EventType::Start && enteredSeconds_ > 0) {
          remainingSeconds_ = enteredSeconds_;
          state_ = State::Running;
        } else if (event.type == EventType::Cancel) {
          reset();
        }
        break;

      case State::Running:
        if (event.type == EventType::Tick) {
          if (remainingSeconds_ > 0) --remainingSeconds_;
          if (remainingSeconds_ == 0) state_ = State::Done;
        } else if (event.type == EventType::Cancel) {
          reset();
        }
        break;

      case State::Done:
        if (event.type == EventType::Cancel || event.type == EventType::Start ||
            event.type == EventType::Digit) {
          reset();
        }
        break;
    }
  }

 private:
  void reset() {
    state_ = State::Idle;
    enteredSeconds_ = 0;
    remainingSeconds_ = 0;
  }

  State    state_ = State::Idle;
  uint16_t enteredSeconds_ = 0;    // built up digit-by-digit while Setting
  uint16_t remainingSeconds_ = 0;  // counts down while Running; holds at 0 through Done
};

}  // namespace microwave
