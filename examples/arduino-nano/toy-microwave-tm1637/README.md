# Toy Microwave (TM1637)

Firmware for a toy microwave control panel: a countdown timer and idle time-of-day clock on a
4-digit TM1637 display, a 4x4 membrane keypad, a buzzer, and on/off outputs for a turntable motor,
a cooling fan, and an interior light. This is the original hardware approach, frozen as a
reference. The live project is `projects/toy-microwave`, which uses an ET6226M to drive the display
and scan the keypad from one chip.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano (`nanoatmega328`), ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/Debounce`, `lib/arduino/KeyMatrix`, `lib/arduino/SevenSeg`, `lib/arduino/ToneSequence`, `lib/arduino/WallClock` |
| Other libraries | [avishorp/TM1637](https://github.com/avishorp/TM1637) (from GitHub) |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests (`env:native`) for `include/Microwave.h` and `include/Buzzer.h`.
- Wokwi simulator (`diagram.json`, `wokwi.toml`).

## Getting started

If `pio` is not on your shell `PATH` (VS Code extension only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

Run from the repo root:

1. Build: `pio run -d examples/arduino-nano/toy-microwave-tm1637`
2. Build and flash the Nano: `pio run -d examples/arduino-nano/toy-microwave-tm1637 -t upload`
3. Run the native tests: `pio test -d examples/arduino-nano/toy-microwave-tm1637 -e native`

## Hardware

| Qty | Role | Part chosen | Notes |
| --- | --- | --- | --- |
| 1 | 4-digit 7-segment display | WWZMDiB 4-Digit 7-Segment LED Display Board | Sold in a 5-pack. TM1637 driver, 2-wire CLK/DIO, 2 Nano pins. |
| 1 | Buzzer | 12085 buzzer, 12 x 8.5 mm, 42 Ω, 3 to 12 V | Sold in a 20-pack. Passive, driven with `tone()`. |
| 1 | Turntable motor | TYC-50 synchronous motor, 12 V DC, 5 to 6 RPM, CW/CCW, 4 W | Matches a real turntable's speed. |
| 1 | Cooling fan | Generic 12 V DC brushless fan, 40 x 40 x 10 mm, sleeve bearing, ~0.08 A | Any 12 V 40 mm fan. Shares the 12 V rail with the motor and light. Needs a transistor or MOSFET driver. |
| 1 | Interior light | 80 mm white COB LED ring ("angel eye") | Sold in a 2-pack. Rated 12 V/24 V. Confirm current draw and switching (relay or MOSFET) before wiring. |
| 1 | Keypad | Generic 4x4 matrix membrane keypad, `1-9,0,*,#,A-D` | A matrix keeps the keypad to 8 pins. Mechanical keyswitches can sit under the same matrix wiring. |

Pin assignments are placeholders. No schematic exists.

| Signal | Nano pin |
| --- | --- |
| Keypad rows 1 to 4 | D9, D8, D7, D6 |
| Keypad columns 1 to 4 | D5, D4, D3, D2 |
| Buzzer | D10 |
| Motor | D11 |
| Fan | D12 |
| Light | D13 |
| Display CLK | A0 |
| Display DIO | A1 |

## How it works

### States

- Idle: shows the time-of-day clock as HH:MM in 12-hour form, with no AM/PM indicator. The clock is
  kept as 24-hour internally. A digit starts a cook time, `B` starts a kitchen timer, and `A`
  starts setting the clock.
- Setting: enters a countdown as MM:SS, calculator style. Each digit shifts in from the right, so
  `3` shows `0:03`, `30` shows `0:30`, and `300` shows `3:00`. Seconds above 59 clamp to 59. `#`
  starts the countdown if the time is nonzero. `*` returns to Idle.
- Running: counts down once per second. A cook turns on the motor, fan, light, and a 120 Hz hum on
  the buzzer. A kitchen timer runs with all of those off. `*` returns to Idle.
- Done: shows " End" with no colon and plays 4 beeps at 2500 Hz, repeated every 10 s until
  acknowledged. The display blanks between beeps and stays lit during the gap between reminders.
  A digit, `#`, or `*` returns to Idle.
- ClockSet: enters a time of day as HH:MM in 24-hour form, so `1930` is 7:30 PM. The whole display
  blinks on a 1000 ms cycle. The preview clamps hours to 23 and minutes to 59. `#` confirms only a
  valid time, so `2400` is refused and entry continues. `*` discards the entry.

The clock advances in every state except ClockSet. Every mapped key plays a 2000 Hz, 40 ms click.

### Keypad mapping

| Key | Action |
| --- | --- |
| `0` to `9` | Enter a digit |
| `#` | Start the countdown, or confirm the clock time |
| `*` | Cancel back to Idle, or end a running countdown |
| `A` | From Idle, set the clock |
| `B` | From Idle, enter a kitchen timer |
| `C`, `D` | Unused |

### Firmware layout

- `include/Microwave.h`: the `Controller` state machine (Idle, Setting, Running, Done, ClockSet),
  driven by `Event`s (`Digit`, `Start`, `Cancel`, `Clock`, `Timer`, `Tick`). Setting, Running,
  and Done serve two `Function`s, `CookTime` and `Timer`. `isCooking()` (Running with `CookTime`)
  is what gates the motor, fan, light, and hum. Time of day and countdown counting come from
  `wallclock::Clock` and `wallclock::Timer`.
- `include/Buzzer.h`: the key press, done, error, and hum sounds as `{frequency, duration}` step
  tables played by `lib/arduino/ToneSequence`. `isFinished()` waits for the whole sequence.
- `lib/arduino/KeyMatrix` and `lib/arduino/Debounce`: `keymatrix::MatrixScanner` drives each row low
  and reads the active column, and `debounce::Debouncer<uint8_t>` settles the key index.
  `main.cpp` looks the index up in `LAYOUT`.
- `lib/arduino/WallClock`: the time-of-day clock, the countdown timer, and digit entry helpers.
- `lib/arduino/SevenSeg`: `encodeText()` for " End", `secondsToDigits()` for the digit pairs, and
  `blinkOn()` for the ClockSet blink. Digits are encoded with the TM1637 library's
  `encodeDigit()`.
- `src/main.cpp`: keypad scan, key to `Event` translation, the 1 s tick, buzzer pattern selection,
  motor, fan, and light outputs, and display rendering. The motor, fan, and light are plain
  `digitalWrite()` calls with no header of their own.

The 1 s tick advances `lastTickMs` by 1000 instead of resetting it to `millis()`, so loop overshoot
does not drift the clock.

## Simulate

The Wokwi diagram has the keypad, the TM1637 display, the buzzer, LEDs standing in for the motor,
fan, and light, and a text label for `A`, `B`, `#`, and `*`. Build, then run
**F1 > Wokwi: Start Simulator** in VS Code and click the keypad keys.

## Known issues

- The colon position (`COLON_DIGIT_INDEX` 1, `COLON_BIT` 0x80 in `src/main.cpp`) is a guess for
  common boards and is not confirmed on this display.
- The clock is software only. It resets to 0:00 on every power loss or reset.
- `buzzer::Pattern::Error` is defined but never played.
- Auto-reset is unreliable on this CH340 Nano. Tap RESET as the upload starts.

## Status

Frozen. The firmware is complete and tested, with 31 native unit tests across `Microwave.h` and
`Buzzer.h`, and no hardware design was made.
