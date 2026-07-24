# arduino-nano/stepper

> Arduino Nano firmware: a potentiometer-controlled stepper motor. The pot sets rotation speed.

This project lives at `arduino-nano/stepper/` in the `embedded` monorepo.

## What it does

One **potentiometer** (A0) sets **rotation speed** (0..`MAX_SPEED_STEPS_PER_SEC`) on a bipolar
stepper motor via a DRV8825/A4988-style driver. Fixed direction — this project is about step-pulse
timing, not direction control or positioning.

This folder is also a testbed comparing two stepper-driving approaches — see below. It's the
general-purpose counterpart to `robot-buoy`'s `Ballast.h`/`Stepper.h`: that project's stepper
usage is a bounded move-to-target stroke (`AccelStepper::moveTo()` + acceleration profile); this
one is continuous, pot-controlled speed (`AccelStepper::setSpeed()`/`runSpeed()`, a different
`AccelStepper` mode) and a from-scratch alternative to it.

## Stepper strategy

`src/main.cpp` has two encapsulated step-generation implementations, selected by one compile-time
constant, `STEP_MODE` (a `StepMode` enum, default `StepMode::Library`):

- **`Library` (default)** — `renderLibrary()`: `AccelStepper`'s constant-speed mode
  (`setSpeed()` + `runSpeed()`). No acceleration curve — steps at a fixed rate until the target
  speed changes — but the library handles all step timing for you.
- **`DirectPulse` (reference implementation)** — `renderDirectPulse()`: toggles the `STEP` pin
  by hand, timed entirely by `micros()`, with no `AccelStepper` involved at all. The interval
  between pulses is `stepper::stepIntervalMicros(speed)` — one line of math
  (`1,000,000 / stepsPerSec`). No library dependency, no acceleration curve, and precise control
  over exactly when each pulse fires.

Unlike the `pwm` project's `DirectTimer` path, both strategies here are just `micros()` +
`digitalWrite()` (or `AccelStepper` calls that do the same under the hood) — no hardware timer
register modes are involved, so both are expected to simulate correctly in Wokwi. Flip
`STEP_MODE` to compare them; the wiring is identical either way.

## Design

- **`include/Stepper.h`** — hardware-free logic (unit-tested via `pio test -e native`), shared by
  both strategies:
  - `potToSpeed()` / `emaStep()` — pot scaling + smoothing (same pattern as `pwm`'s `Pwm.h`)
  - `stepIntervalMicros(stepsPerSec)` — target speed → microseconds between pulses
    (`DirectPulse` path only; `Library` path passes speed straight to `AccelStepper`)
- **`src/main.cpp`** — pins, both strategies as encapsulated functions, the loop (read pot →
  speed → whichever strategy is selected). `DEBUG_TRACE_ENABLED` (off by default) prints a
  Teleplot-format trace (`raw`/`speed`) for debugging pot→speed correspondence.

Kept deliberately small — single-responsibility functions (ADC→speed, speed→interval) and no
class hierarchies or interfaces.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/stepper
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Potentiometer wiper | A0 | outer terminals to 5V/GND, wiper to A0 |
| Driver `DIR` | D2 | fixed HIGH at boot — one direction only |
| Driver `STEP` | D3 | |
| Driver `ENABLE`, `MS1`, `MS2`, `MS3` | — | hardwired to GND on the driver board (always enabled, full-step) — not driven by the MCU |

Same wiring for both strategies — only the step-generation code differs, not the pins.

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (A4988 + stepper motor, same substitution `robot-buoy`
uses for its real DRV8825). Build (`pio run`), then **F1 → "Wokwi: Start Simulator"**. Drag the
potentiometer to see the motor's rotation speed change.
