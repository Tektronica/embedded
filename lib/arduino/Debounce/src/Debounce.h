#pragma once

#include <stdint.h>

// Hardware-free debouncing: a value only changes after the same new reading has been sampled
// `samples` times in a row. Works for any type with ==: a button level, a raw key index, a key
// position struct.
namespace debounce {

constexpr uint8_t DEFAULT_SAMPLES = 3;

template <typename T>
class Debouncer {
 public:
  explicit Debouncer(T initial, uint8_t samples = DEFAULT_SAMPLES)
      : state_(initial), candidate_(initial), samples_(samples) {}

  // Feed one raw sample. Returns true on the sample that commits a new value.
  bool update(T raw) {
    if (raw == state_) {
      count_ = 0;
      return false;
    }
    if (!(raw == candidate_)) {
      candidate_ = raw;
      count_ = 0;
    }
    if (++count_ < samples_) return false;
    state_ = raw;
    count_ = 0;
    return true;
  }

  T value() const { return state_; }

 private:
  T state_;
  T candidate_;
  uint8_t samples_;
  uint8_t count_ = 0;
};

// Push button: true once per debounced press (the committed change to pressed), never on
// release or while held.
class Button {
 public:
  explicit Button(uint8_t samples = DEFAULT_SAMPLES) : debouncer_(false, samples) {}

  bool pressed(bool raw) { return debouncer_.update(raw) && debouncer_.value(); }

 private:
  Debouncer<bool> debouncer_;
};

}  // namespace debounce
