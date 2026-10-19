# Buzzer Song

A passive buzzer and one momentary pushbutton. The buzzer loops one of five tracks, and each press advances to the next: a scale and four borrowed songs. Playback is non-blocking and reads long songs straight from flash. `examples/arduino-nano/buzzer-simple/` is the naive blocking counterpart.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz (`nanoatmega328`) |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/Debounce`, `lib/arduino/ToneSequence` |
| Other libraries | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests (`env:native`) for `include/Song.h`.
- Wokwi simulator (`wokwi.toml`, `diagram.json`).

## Getting started

Run from the repo root. If `pio` is not on your shell `PATH` (VS Code extension only installs), add it first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d examples/arduino-nano/buzzer-song`
2. Flash the Nano: `pio run -d examples/arduino-nano/buzzer-song -t upload`
3. Run the unit tests: `pio test -d examples/arduino-nano/buzzer-song -e native`
4. Open the serial monitor: `pio device monitor -d examples/arduino-nano/buzzer-song`

Auto-reset is unreliable on this CH340 board. Tap RESET as the upload starts. Serial output only appears when `DEBUG_TRACE_ENABLED` is set to `true` in `src/main.cpp`.

## Hardware

- Arduino Nano
- Passive buzzer
- Momentary pushbutton

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Passive buzzer | D9 | Driven by `tone()` and `noTone()`, other leg to GND |
| Track button | D2 | `INPUT_PULLUP`, other leg to GND. Each fresh press advances Scale, Tetris, Mario, Doom, Nokia, then wraps |

## How it works

| Track | Source | Notes |
| --- | --- | --- |
| Scale | Original | C4 to C5 quarter notes at 200 BPM, repeating |
| Tetris | Traditional, "Korobeiniki", public domain | Full theme, 97 notes |
| Mario | Koji Kondo, Nintendo, 1985 | "Overworld" theme, 321 notes. Personal, non-commercial hobby use only. Copyright remains with Nintendo |
| Doom | Bobby Prince, id Software, 1993 | "At Doom's Gate" (E1M1), 680 notes. Personal, non-commercial hobby use only. Copyright remains with id Software |
| Nokia | Traditional, public domain, popularized by Nokia | Default ringtone excerpt, 13 notes |

Tetris, Mario, Doom, and Nokia were transcribed from [robsoncouto/arduino-songs](https://github.com/robsoncouto/arduino-songs), which states no license.

### Files

- `include/Notes.h`: hardware-free equal-temperament note constants in Hz. Octaves 3 to 6 are fully populated, the practical range for a small piezo. Octave 2 holds only E2, A2, A#2, and B2, added for Doom's bass line.
- `include/Song.h`: hardware-free and unit-tested. Defines `Note{frequencyHz, durationDivider}` with the tracker convention (4 is a quarter, 8 an eighth, negative is dotted), `durationMs(divider, tempoBpm)`, the `Track` enum with `next()`, and `toneStateFor(notes, count, tempoBpm, elapsedMs)`. Playback loops the sequence and sounds each note for `NOTE_ON_PERCENT` (90%) of its duration, so repeated same-pitch notes stay separate. The Scale table lives here and stays in RAM.
- `include/songs/Tetris.h`, `Mario.h`, `Doom.h`, `Nokia.h`: one song per file. Each holds a `PROGMEM` `NOTES` table, `TEMPO_BPM`, `COUNT`, and its attribution comment.
- `include/SongPlayer.h`: hardware-coupled, needs `avr/pgmspace.h`, so it has no native tests. `toneStateForProgmem()` runs the same algorithm as `song::toneStateFor()` but reads each note with `pgm_read_word_near` and `pgm_read_byte_near`. It reuses `song::durationMs()` so both storage paths time notes the same way. `songplayer::update(currentTrack, elapsedMs)` is the single entry point `main.cpp` calls.
- `lib/arduino/Debounce`: `debounce::Button`, the debounced press edge detector.
- `src/main.cpp`: pins, one `Button` that advances `currentTrack` with `song::next()` and restarts the track clock, and the loop that maps `songplayer::update()` to `tone()` or `noTone()`. `DEBUG_TRACE_ENABLED` (off by default) prints a Teleplot-format trace of the track and tone state at 115200 baud every 200 ms.

### PROGMEM

The ATmega328P has 2 KB of SRAM. AVR copies ordinary `const` globals from flash into RAM at boot, so Tetris (97 notes), Mario (321), and Doom (680) would not fit as plain arrays. `PROGMEM` keeps them in flash. Nokia (13 notes) would fit in RAM but uses `PROGMEM` too, so every borrowed song shares one storage path.

### Adding a song

A new song needs a header under `include/songs/` with `NOTES`, `TEMPO_BPM`, and `COUNT`, one new `Track` enum value, one `switch` case in `SongPlayer.h`, and an updated modulus in `song::next()`. The playback functions do not change.

`docs/transpiling-songs.md` walks through the source format and how `scripts/transpile-song.sh` converts an arduino-songs sketch into a `songs/<Song>.h` note table. Tetris, Mario, Doom, and Nokia were added this way.

### Note frequency reference

`Notes.h` defines only octaves 3 to 6 plus four octave 2 notes. This is the full equal-temperament chart for reference.

| Note | Octave 0 | Octave 1 | Octave 2 | Octave 3 | Octave 4 | Octave 5 | Octave 6 | Octave 7 | Octave 8 |
|---|---|---|---|---|---|---|---|---|---|
| C | 16.35 Hz | 32.70 Hz | 65.41 Hz | 130.81 Hz | 261.63 Hz | 523.25 Hz | 1046.50 Hz | 2093.00 Hz | 4186.01 Hz |
| C#/Db | 17.32 Hz | 34.65 Hz | 69.30 Hz | 138.59 Hz | 277.18 Hz | 554.37 Hz | 1108.73 Hz | 2217.46 Hz | 4434.92 Hz |
| D | 18.35 Hz | 36.71 Hz | 73.42 Hz | 146.83 Hz | 293.66 Hz | 587.33 Hz | 1174.66 Hz | 2349.32 Hz | 4698.63 Hz |
| D#/Eb | 19.45 Hz | 38.89 Hz | 77.78 Hz | 155.56 Hz | 311.13 Hz | 622.25 Hz | 1244.51 Hz | 2489.02 Hz | 4978.03 Hz |
| E | 20.60 Hz | 41.20 Hz | 82.41 Hz | 164.81 Hz | 329.63 Hz | 659.25 Hz | 1318.51 Hz | 2637.02 Hz | 5274.04 Hz |
| F | 21.83 Hz | 43.65 Hz | 87.31 Hz | 174.61 Hz | 349.23 Hz | 698.46 Hz | 1396.91 Hz | 2793.83 Hz | 5587.65 Hz |
| F#/Gb | 23.12 Hz | 46.25 Hz | 92.50 Hz | 185 Hz | 369.99 Hz | 739.99 Hz | 1479.98 Hz | 2959.96 Hz | 5919.91 Hz |
| G | 24.50 Hz | 49 Hz | 98 Hz | 196 Hz | 392 Hz | 783.99 Hz | 1567.98 Hz | 3135.96 Hz | 6271.93 Hz |
| G#/Ab | 25.96 Hz | 51.91 Hz | 103.83 Hz | 207.65 Hz | 415.30 Hz | 830.61 Hz | 1661.22 Hz | 3322.44 Hz | 6644.88 Hz |
| A | 27.50 Hz | 55 Hz | 110 Hz | 220 Hz | 440 Hz | 880 Hz | 1760 Hz | 3520 Hz | 7040 Hz |
| A#/Bb | 29.14 Hz | 58.27 Hz | 116.54 Hz | 233.08 Hz | 466.16 Hz | 932.33 Hz | 1864.66 Hz | 3729.31 Hz | 7458.62 Hz |
| B | 30.87 Hz | 61.74 Hz | 123.47 Hz | 246.94 Hz | 493.88 Hz | 987.77 Hz | 1975.53 Hz | 3951.07 Hz | 7902.13 Hz |

## Simulate

The diagram holds a `wokwi-buzzer` and one pushbutton.

1. Build: `pio run -d examples/arduino-nano/buzzer-song`
2. In VS Code, run F1, then "Wokwi: Start Simulator".
3. Press the button to cycle through Scale, Tetris, Mario, Doom, and Nokia.

## Status

Builds, with native tests for `Song.h`. Not yet verified on real hardware.
