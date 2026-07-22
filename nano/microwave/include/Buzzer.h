#pragma once

#include <stdint.h>

// Passive buzzer: tone/frequency selection and beep-pattern sequencing (key press, done, error).
// Hardware-free so it unit-tests off-device; main.cpp drives the actual tone()/PWM output.
namespace buzzer {

// TODO: beep-pattern timing (key-press tone, done tone, error tone).

}  // namespace buzzer
