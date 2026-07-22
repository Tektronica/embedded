# nano/microwave

Arduino Nano firmware for a toy/prop microwave oven control panel: a countdown timer on a 4-digit
display, a turntable motor, a done/alert buzzer, a cooling fan and buzzer-tone layer for the
running "hum," an interior light, and a 4x4 matrix keypad for setting time and start/stop.

## BOM

| Qty | Role (generic) | Part chosen | Notes |
|---|---|---|---|
| 1 | 4-digit 7-segment display (countdown timer) | WWZMDiB 4-Digit 7-Segment LED Display Board for Arduino | Sold in a 5-pack. TM1637 driver chip — 2-wire CLK/DIO protocol, only 2 Nano pins needed. |
| 1 | Buzzer (done/alert tone, button feedback, running hum) | 12085 buzzer, 12×8.5mm, 42Ω, 3–12V | Sold in a 20-pack. Passive — driven via `tone()`/PWM; also layers a quiet low-frequency tone during Running, alongside the fan and motor, for the ambient hum. |
| 1 | Turntable motor | TYC-50 synchronous motor, 12V DC, 5–6 RPM, CW/CCW, 4W | Matches a real microwave turntable's rotation speed; its own running sound also contributes to the hum. |
| 1 | Cooling fan (running hum) | Generic 12V DC brushless fan, 40×40×10mm, sleeve bearing, ~0.08A | A commodity spec, not a specific listing — any 12V 40mm fan matches. Matches the 12V rail already used by the motor/light rather than introducing a separate 5V domain. Needs a transistor/MOSFET driver, same as the motor — not a direct GPIO connection. |
| 1 | Interior/status light | 80mm white COB LED ring ("angel eye" halo ring) | Sold in a 2-pack. 12V/24V rated — confirm current draw and how it's switched (relay/MOSFET vs. direct GPIO) before wiring. |
| 1 | Keypad (time entry, start/stop) | Generic 4x4 matrix membrane keypad (16-key, `1-9,0,*,#,A-D` layout) | Chosen as a matrix (not individually-wired mechanical switches) to keep the pin count to 8 (4 rows + 4 cols) alongside the display/motor/fan/light. Mechanical keyswitches (Cherry MX or similar) can still sit under a matrix if tactile feel matters — the matrix wiring is the same either way. |

## Firmware

Self-contained PlatformIO project (`platformio.ini`, `include/`, `src/`, `test/`), fully wired up:

- **`SevenSegment.h`** — countdown-time formatting (seconds → MM:SS digit values) and a blink-cycle helper, used for the colon while Setting/Done.
- **`Buzzer.h`** — tone/frequency selection and beep-pattern timing (key press, done — 4 beeps, error) plus the continuous Running hum, and `isFinished()` to know when a one-shot pattern's whole sequence (not just its current on/off phase) has completed.
- **`KeyMatrix.h`** — debounced key detection across a 4x4 matrix, using the same debounce pattern as `nano/ledStripDimmer`'s `Button` class generalized to a 16-way key index. Named to avoid colliding with the well-known Arduino `Keypad` library.
- **`Microwave.h`** — the Idle → Setting → Running → Done state machine, driven by abstract `Event`s (`Digit`/`Start`/`Cancel`/`Tick`) rather than any specific keypad wiring.
- **`main.cpp`** — matrix scan → debounced key → `Event` → state machine → buzzer pattern selection, motor/fan/light on/off, and TM1637 display rendering.
- Motor, fan, and light have no dedicated header — all three are plain on/off hardware glue in `main.cpp`.

**Display library**: `avishorp/TM1637` pulled directly from GitHub (see `platformio.ini`'s
`lib_deps` — the PlatformIO registry re-registration has an odd unversioned release, so this
points at the actual upstream repo instead). The colon segment's exact bit position
(`COLON_DIGIT_INDEX`/`COLON_BIT` in `main.cpp`) is a common-board guess, not yet confirmed against
this specific display — check it once the display is wired up and adjust if the colon doesn't light.

Pin assignments (placeholder pending a real schematic): matrix rows D2–D5, columns D6–D9, buzzer
D10, motor D11, fan D12, light D13, display CLK A0 / DIO A1.

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` cover the keypad, display, buzzer, and motor/fan/light LEDs. Build
first (`pio run`), then **F1 → "Wokwi: Start Simulator"** and click the matrix keypad's keys to
drive the state machine and watch the countdown on the display.

## Status

BOM set. No hardware design yet. Firmware logic is implemented and tested (26 unit tests across
the four headers) and wired end-to-end, including the TM1637 display.
