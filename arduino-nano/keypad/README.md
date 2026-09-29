# arduino-nano/keypad

> Arduino Nano firmware: a debounced 4x4 matrix keypad reader. Each key press prints its
> character over Serial and pulses an LED.

This project lives at `arduino-nano/keypad/` in the `embedded` monorepo.

## What it does

A standard 16-key membrane keypad (`1-9`, `0`, `*`, `#`, `A`-`D`) is scanned continuously; each
fresh, debounced key press is printed to Serial and briefly lights an LED.

This is the standalone counterpart to `arduino-nano/toy-microwave`'s identical keypad-reading
technique — that project consumes it for one specific application (a cook-timer's digit entry);
this project is where the technique itself lives and gets exercised on its own, the same
relationship `pwm-pot-demo`/`stepper` have to the projects that actually use PWM/stepper control for
something.

## Design

This project is the example app for two shared libraries:

- **`lib/KeyMatrix`** — `keymatrix::MatrixScanner`, the row/column GPIO scan: drive each row low
  in turn, read back which column (if any) is pulled low, return a raw index or `NO_KEY`. Takes
  its row/column counts from the app. Hardware-coupled, so it doesn't unit-test off-device.
- **`lib/Debounce`** — `debounce::Debouncer<uint8_t>`, which commits a raw index only after the
  same reading repeats for 3 scans in a row.
- **`src/main.cpp`** — pins, the 4x4 `LAYOUT`, the LED pulse, and the loop: scan → debounce →
  look up the settled index in `LAYOUT`.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/keypad
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor (9600 baud) -- shows each key pressed
```

Unit tests live with the libraries: `pio test -d lib/Debounce` from the repo root.

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Keypad rows (R1-R4) | D9-D6 | driven low one at a time during scanning |
| Keypad columns (C1-C4) | D5-D2 | `INPUT_PULLUP`, read low when a key bridges row to column |
| Indicator LED | D10 | through a ~330 Ω resistor; pulses ~150 ms per debounced keypress |

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (`wokwi-membrane-keypad`, same part `toy-microwave`
uses, plus the indicator LED). Build (`pio run`), then **F1 → "Wokwi: Start Simulator"** and
click the keypad's keys — each press prints to the Serial monitor and blinks the LED.

## Status

Builds against `lib/KeyMatrix` and `lib/Debounce` (tested in `lib/Debounce`); not yet verified
against real hardware.
