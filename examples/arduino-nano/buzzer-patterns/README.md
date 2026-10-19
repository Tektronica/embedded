# Buzzer Patterns

A passive buzzer and three momentary pushbuttons. Each button plays a named one-shot alert pattern (KeyPress, Done, Error) without blocking. `projects/toy-microwave` and `examples/arduino-nano/toy-microwave-tm1637` each carry a copy of the same `Buzzer.h` pattern tables for key-press feedback and a done chime.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz (`nanoatmega328`) |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/Debounce`, `lib/arduino/ToneSequence` |
| Other libraries | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests (`env:native`) for `include/Buzzer.h`.
- Wokwi simulator (`wokwi.toml`, `diagram.json`).

## Getting started

Run from the repo root. If `pio` is not on your shell `PATH` (VS Code extension only installs), add it first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d examples/arduino-nano/buzzer-patterns`
2. Flash the Nano: `pio run -d examples/arduino-nano/buzzer-patterns -t upload`
3. Run the unit tests: `pio test -d examples/arduino-nano/buzzer-patterns -e native`
4. Open the serial monitor: `pio device monitor -d examples/arduino-nano/buzzer-patterns`

Auto-reset is unreliable on this CH340 board. Tap RESET as the upload starts. Serial output only appears when `DEBUG_TRACE_ENABLED` is set to `true` in `src/main.cpp`.

## Hardware

- Arduino Nano
- Passive buzzer
- Three momentary pushbuttons

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Passive buzzer | D9 | Driven by `tone()` and `noTone()`, other leg to GND |
| KeyPress button | D2 | `INPUT_PULLUP`, other leg to GND |
| Done button | D3 | `INPUT_PULLUP`, other leg to GND |
| Error button | D4 | `INPUT_PULLUP`, other leg to GND |

## How it works

| Button | Pattern | Sound |
| --- | --- | --- |
| KeyPress | `Pattern::KeyPress` | One 2 kHz blip, 40 ms |
| Done | `Pattern::Done` | Four 2.5 kHz beeps of 200 ms, each followed by a 150 ms gap |
| Error | `Pattern::Error` | One 300 Hz tone, 400 ms |

A press starts its pattern from the beginning, even if a pattern is already playing. The buzzer goes silent when the pattern finishes. Playback is non-blocking, with no `delay()` calls.

- `include/Buzzer.h`: hardware-free and unit-tested. Holds the `Pattern` enum (`None`, `KeyPress`, `Done`, `Error`) and one `{frequency, duration}` step table per pattern. `toneStateFor(pattern, elapsedMs)` and `isFinished(pattern, elapsedMs)` wrap `lib/arduino/ToneSequence`.
- `lib/arduino/Debounce`: `debounce::Button`, the debounced press edge detector, one instance per button.
- `src/main.cpp`: pins, the three buttons, and the loop. The loop maps `buzzer::toneStateFor()` to `tone()` or `noTone()` and resets to `Pattern::None` once `isFinished()` returns true. The only state is the active pattern and its start time.

`DEBUG_TRACE_ENABLED` (off by default) prints a Teleplot-format trace of the active pattern and tone state at 115200 baud every 200 ms.

## Simulate

The diagram holds a `wokwi-buzzer` and three pushbuttons.

1. Build: `pio run -d examples/arduino-nano/buzzer-patterns`
2. In VS Code, run F1, then "Wokwi: Start Simulator".
3. Press KEYPRESS, DONE, or ERROR to hear each pattern.

## Status

Builds, with native tests for `Buzzer.h`. Not yet verified on real hardware.
