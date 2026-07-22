#pragma once

#include <stdint.h>

// Top-level control-flow state machine: Idle -> Setting -> Running -> Done. Coordinates the
// display, buzzer, keypad, motor, and light without touching any hardware directly, so it
// unit-tests off-device; main.cpp wires it to the actual peripherals.
namespace microwave {

// TODO: state enum (Idle, Setting, Running, Done) + transition logic.

}  // namespace microwave
