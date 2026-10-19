#pragma once

#include <stdint.h>

// Equal-temperament note frequencies (Hz). Octaves 3-6 are the practical range for a small piezo
// buzzer (lower is inaudible/inefficient on a piezo element, higher exceeds typical piezo
// response) and fully populated below. Octave 2 is added on demand, one pitch at a time, only as
// a song actually uses it (currently Doom's bass line) -- not the full octave, to avoid unused
// constants. See README for the full C0-B8 reference chart. Sharps are named with an `s`
// (`Cs4` = C#4/Db4, an enharmonic pair sharing one frequency) since `#` isn't a valid identifier
// character.
namespace notes {

constexpr uint16_t REST = 0;  // silence, not a pitch

// Octave 2 (partial) -- see comment above.
constexpr uint16_t E2 = 82, A2 = 110, As2 = 117, B2 = 123;

constexpr uint16_t C3 = 131, Cs3 = 139, D3 = 147, Ds3 = 156, E3 = 165, F3 = 175,
                    Fs3 = 185, G3 = 196, Gs3 = 208, A3 = 220, As3 = 233, B3 = 247;

constexpr uint16_t C4 = 262, Cs4 = 277, D4 = 294, Ds4 = 311, E4 = 330, F4 = 349,
                    Fs4 = 370, G4 = 392, Gs4 = 415, A4 = 440, As4 = 466, B4 = 494;

constexpr uint16_t C5 = 523, Cs5 = 554, D5 = 587, Ds5 = 622, E5 = 659, F5 = 698,
                    Fs5 = 740, G5 = 784, Gs5 = 831, A5 = 880, As5 = 932, B5 = 988;

constexpr uint16_t C6 = 1047, Cs6 = 1109, D6 = 1175, Ds6 = 1245, E6 = 1319, F6 = 1397,
                    Fs6 = 1480, G6 = 1568, Gs6 = 1661, A6 = 1760, As6 = 1865, B6 = 1976;

}  // namespace notes
