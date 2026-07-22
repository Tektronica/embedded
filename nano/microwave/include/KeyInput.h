#pragma once

#include <stdint.h>

// Keypad input: debounced key events for time entry and start/stop/cancel. Hardware-free so it
// unit-tests off-device. Contents pending the matrix-vs-individually-wired-switches decision — a
// matrix needs scan + debounce logic; individually-wired switches can reuse a plain per-key
// debounce (see nano/ledStripDimmer's Button class for that pattern).
namespace keyinput {

// TODO: key debounce (+ matrix scan, if wired as a matrix).

}  // namespace keyinput
