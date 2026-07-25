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
relationship `pwm`/`stepper` have to the projects that actually use PWM/stepper control for
something.

## Design

- **`include/KeyMatrix.h`** — hardware-free (unit-tested via `pio test -e native`): given a raw
  key index, debounces it (N consistent scans before committing, same pattern as
  `led-dimmer-ws2812`'s `Button` class) and looks up the settled key's character from the
  standard membrane layout.
- **`include/MatrixScanner.h`** — the generic row/column GPIO scan technique: drive each row low
  in turn, read back which column (if any) is pulled low. Knows nothing about debouncing or key
  characters — pure physical scanning, reusing `KeyMatrix.h`'s `ROWS`/`COLS`/`NO_KEY` constants
  so the two stay in sync. Hardware-coupled (`digitalWrite`/`digitalRead`), so unlike
  `KeyMatrix.h` this doesn't unit-test off-device — same shape as `stepper`'s `Stepper.h` and
  `robot-buoy`'s `Radio.h`: a generic hardware driver in its own header, separate from the
  hardware-free logic it feeds.
- **`src/main.cpp`** — pins, the LED pulse, and the loop that connects
  `matrixscanner::Scanner::scan()`'s raw index to `keymatrix::Scanner::scan()`'s debounced
  character.

Kept deliberately small — two single-responsibility classes (raw scanning, debounce+lookup) and
one thin `main.cpp` tying them together, no deeper hierarchy.

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
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor (9600 baud) -- shows each key pressed
```

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

Built and tested (6/6 native unit tests); not yet verified against real hardware.
