# arduino-nano/buzzer-simple

> Arduino Nano firmware: a passive buzzer and a momentary pushbutton. Press the button, hear a
> beep. Deliberately the naive version — see `arduino-nano/buzzer-song/` for the non-blocking,
> multi-song counterpart this project exists to contrast against.

This project lives at `arduino-nano/buzzer-simple/` in the `embedded` monorepo.

## What it does

While the button is held, the buzzer beeps at a fixed pitch (`BEEP_HZ`) for `BEEP_MS`, using
`tone()` and a blocking `delay()` — the same style as a first "hello world" buzzer sketch, and the
same style `arduino-nano/buzzer-song`'s `Song`/`SongPlayer` design specifically replaces.

## Design

Everything lives in `src/main.cpp` — no `include/` headers, no unit tests. That's intentional,
not an oversight: this project has no hardware-free logic to extract or test. `delay()` blocks
the whole MCU (no other work can happen while a beep plays), and there's no debouncing (a held
button just beeps repeatedly, gated by `BEEP_MS`'s own delay rather than a proper edge detector).
Both are real limitations — see `buzzer-song`'s README for how it avoids each: a `millis()`-polled
non-blocking `ToneState`, and a debounced `Button` class for clean single-press edges.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/buzzer-simple
pio run                 # build
pio run -t upload       # build + flash the Nano
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Passive buzzer | D9 | driven via `tone()`/`noTone()`, other leg to GND |
| Beep button | D2 | `INPUT_PULLUP`, other leg to GND |

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (`wokwi-buzzer` + a pushbutton). Build (`pio run`),
then **F1 → "Wokwi: Start Simulator"** and hold the button to hear the beep.

## Status

Built; not yet verified against real hardware.
