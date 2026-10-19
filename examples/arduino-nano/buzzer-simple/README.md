# Buzzer Simple

A passive buzzer and one momentary pushbutton. Holding the button beeps the buzzer with blocking `tone()` and `delay()` calls. This is the naive baseline that `examples/arduino-nano/buzzer-song/` replaces with a non-blocking, debounced design.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz (`nanoatmega328`) |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | None |
| Other libraries | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Wokwi simulator (`wokwi.toml`, `diagram.json`).

## Getting started

Run from the repo root. If `pio` is not on your shell `PATH` (VS Code extension only installs), add it first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d examples/arduino-nano/buzzer-simple`
2. Flash the Nano: `pio run -d examples/arduino-nano/buzzer-simple -t upload`

Auto-reset is unreliable on this CH340 board. Tap RESET as the upload starts.

## Hardware

- Arduino Nano
- Passive buzzer
- Momentary pushbutton

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Passive buzzer | D9 | Driven by `tone()` and `noTone()`, other leg to GND |
| Beep button | D2 | `INPUT_PULLUP`, other leg to GND |

## How it works

While the button reads LOW, the loop plays `BEEP_HZ` (440 Hz) for `BEEP_MS` (200 ms), stops the tone, then waits another 200 ms. A held button therefore beeps repeatedly at 2.5 Hz.

All code is in `src/main.cpp`. There is no `include/` split and no unit tests, because there is no hardware-free logic to extract. The design has two limitations, both on purpose:

- `delay()` blocks the MCU, so nothing else runs while a beep plays.
- There is no debounce or edge detection. The second `delay()` is the only gate between beeps.

`buzzer-song` removes both: it polls `millis()` for non-blocking playback and uses `debounce::Button` from `lib/arduino/Debounce` for clean single-press edges.

## Simulate

The diagram holds a `wokwi-buzzer` and one pushbutton.

1. Build: `pio run -d examples/arduino-nano/buzzer-simple`
2. In VS Code, run F1, then "Wokwi: Start Simulator".
3. Hold the button to hear the beep.

## Status

Builds. Not yet verified on real hardware.
