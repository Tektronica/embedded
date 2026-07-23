# arduino-nano/pwm

> Arduino Nano firmware: a potentiometer-controlled PWM output. The pot sets duty cycle (0–100%).

This project lives at `arduino-nano/pwm/` in the `embedded` monorepo.

## What it does

One **potentiometer** (A0) sets **duty cycle** 0–100% on the PWM output (D9).

This folder is also a testbed comparing two PWM implementation approaches — see below.

## PWM pins (ATmega328P)

The Nano's 6 PWM-capable pins split across 3 timers, each with its own default `analogWrite()`
frequency:

| Timer | Pins | Frequency |
|---|---|---|
| Timer0 | D5, D6 | ~980 Hz (also drives `millis()`/`delay()`) |
| Timer1 | D9, D10 | ~490 Hz |
| Timer2 | D3, D11 | ~490 Hz |

This project uses D9 (Timer1).

## PWM strategy

`src/main.cpp` has two encapsulated PWM implementations, selected by one compile-time constant,
`PWM_MODE` (a `PwmMode` enum, default `PwmMode::AnalogWrite`):

- **`analogWrite()` (default)** — `renderAnalogWritePwm()`: fixed at Timer1's default frequency
  (~490 Hz on D9/D10). Simplest option, and what's actually used here — the LEDs look fine at that
  frequency and nothing in this project needs a specific one.
- **DirectTimer (reference implementation)** — `setupDirectTimerPwm()` / `renderDirectTimerPwm()`:
  Timer1 Fast PWM mode 14 (`ICR1` as TOP), for when an arbitrary settable frequency actually
  matters (motors, audio, avoiding interference). `PWM_FREQUENCY_HZ` (default 1000 Hz) is one
  constant to change for a different frequency. Correct per the ATmega328 datasheet, but **not
  simulator-friendly** — Wokwi's AVR core ([avr8js](https://github.com/wokwi/avr8js)) doesn't
  correctly emulate Timer1 modes where TOP is read from a register (`ICR1` or `OCR1A`) rather than
  being a fixed hardware constant — the pin stays stuck high regardless of the duty compare
  register. This was confirmed with two independent approaches (Fast PWM mode 14 with `ICR1` as
  TOP, then mode 15 with `OCR1A` as TOP) that both failed identically in Wokwi. It's a known
  category of gap, not specific to this project — see
  [avr8js#122](https://github.com/wokwi/avr8js/issues/122) and the
  [simavr equivalent](https://github.com/buserror/simavr/issues/162) in a different AVR simulator.
  `analogWrite()` uses Timer1's Phase-Correct mode with a *hardcoded* TOP (`0x00FF`) — the most
  heavily-exercised PWM path in the whole Arduino ecosystem, so it's reliably emulated everywhere.

If you flip `PWM_MODE` to `PwmMode::DirectTimer` to test it, that's a real-hardware-only path —
don't expect it to behave correctly in the Wokwi simulator.

## Design

- **`include/Pwm.h`** — hardware-free logic (unit-tested via `pio test -e native`), shared by both
  PWM strategies:
  - `dutyFromAdc()` / `emaStep()` — pot scaling + smoothing
  - `gammaCorrect(percent)` — square-law correction for real-LED/eye perceptual nonlinearity (a
    linear duty sweep looks maxed out well before 100% on real hardware). Defined and tested but
    **not called by default** — Wokwi's simulated LED rendering already applies its own brightness
    curve, so stacking this on top over-suppressed the bottom half of the pot's range. Wire it back
    in (`pwm::gammaCorrect(pwm::dutyFromAdc(...))` in the loop) if driving a real LED.
  - `dutyToPwm8(dutyPercent)` — duty% → `analogWrite()` value, 0..255 (default path)
  - `computeTimerConfig(cpuHz, targetHz)` / `dutyToOcr(dutyPercent, top)` — frequency → Timer1
    prescaler + TOP, and duty% → `OCR1A` compare value (DirectTimer reference path)
- **`src/main.cpp`** — pins, both PWM strategies as encapsulated functions, the loop (read pot →
  duty → whichever strategy is selected). `DEBUG_TRACE_ENABLED` (off by default) prints a
  Teleplot-format trace (`raw`/`duty`) for debugging pot→PWM correspondence.

Kept deliberately small — single-responsibility functions (ADC→duty, frequency→registers,
duty→register) and no class hierarchies or interfaces.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/pwm
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Potentiometer wiper | A0 | outer terminals to 5V/GND, wiper to A0 |
| PWM output | D9 | through a ~330 Ω resistor if driving an LED |

Same wiring for both PWM strategies — only the register setup differs, not the pin.

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included — build (`pio run`, the default `PwmMode::AnalogWrite`
mode simulates correctly), then **F1 → "Wokwi: Start Simulator"**. Drag the potentiometer to see
the LED brightness change with duty cycle.
