# Keypad

A debounced 4x4 matrix keypad reader. Each fresh key press prints its character over Serial and pulses an LED. This is the example app for `lib/arduino/KeyMatrix`, the same scanning code `examples/arduino-nano/toy-microwave-tm1637` uses for digit entry.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz (`nanoatmega328`) |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/Debounce`, `lib/arduino/KeyMatrix` |
| Other libraries | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Wokwi simulator (`wokwi.toml`, `diagram.json`).

## Getting started

Run from the repo root. If `pio` is not on your shell `PATH` (VS Code extension only installs), add it first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d examples/arduino-nano/keypad`
2. Flash the Nano: `pio run -d examples/arduino-nano/keypad -t upload`
3. Open the serial monitor at 9600 baud: `pio device monitor -d examples/arduino-nano/keypad`

Auto-reset is unreliable on this CH340 board. Tap RESET as the upload starts. This example has no tests of its own. The debouncer is tested in its library: `pio test -d lib/arduino/Debounce`.

## Hardware

- Arduino Nano
- 16-key membrane keypad (`1` to `9`, `0`, `*`, `#`, `A` to `D`)
- LED and 330 Ω resistor

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Keypad rows R1 to R4 | D9, D8, D7, D6 | Driven low one at a time during the scan |
| Keypad columns C1 to C4 | D5, D4, D3, D2 | `INPUT_PULLUP`, read low when a key bridges row to column |
| Indicator LED | D10 | Through a 330 Ω resistor, lit for 150 ms per debounced press |

## How it works

The loop scans the matrix continuously, debounces the raw index, and looks up the settled index in a 4x4 `LAYOUT` table.

- `lib/arduino/KeyMatrix`: `keymatrix::MatrixScanner` drives each row low in turn and reads back which column, if any, is pulled low. It returns a raw key index or `NO_KEY`. The app supplies the pins and the row and column counts. The scanner is hardware-coupled, so it has no off-device tests.
- `lib/arduino/Debounce`: `debounce::Debouncer<uint8_t>` commits a raw index only after the same reading repeats for 3 scans in a row.
- `src/main.cpp`: pins, `LAYOUT`, and the LED pulse. On each committed index other than `NO_KEY`, it prints the character with `Serial.println()` and turns the LED on until `millis()` passes the 150 ms deadline.

## Simulate

The diagram holds a `wokwi-membrane-keypad`, an LED, and a 330 Ω resistor.

1. Build: `pio run -d examples/arduino-nano/keypad`
2. In VS Code, run F1, then "Wokwi: Start Simulator".
3. Click keypad keys. Each press prints to the serial monitor and blinks the LED.

## Status

Builds. Not yet verified on real hardware.
