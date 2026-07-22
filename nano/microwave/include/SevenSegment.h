#pragma once

#include <stdint.h>

// 4-digit 7-segment display (TM1637 driver board, 2-wire CLK/DIO protocol). The TM1637 chip
// handles digit -> segment-pattern encoding and multiplexing itself, via a library, so this
// header's job is countdown-time formatting instead: seconds -> the four digit values (and any
// blink/colon state) to hand to that library. Hardware-free so it unit-tests off-device.
namespace sevenseg {

// TODO: seconds -> MM:SS digit values, blink state while paused/done.

}  // namespace sevenseg
